<?php

/**
 * MySQL aggregation based clustering for geocluster.
 */
class GeoclusterMySQLGeoclusterAlgorithm extends GeoclusterAlgorithm {

  function __construct($config, $cluster_distance, $zoom, $field_handler) {
    parent::__construct($config, $cluster_distance, $zoom, $field_handler);
  }

    /**
   * Add geocluster-based aggregation settings before executing the view.
   */
  function pre_execute() {
    if($this->config->get_option('group_by')) {
      $view = $this->config->get_view();
      foreach ($view->field as $field_key => $field) {
        if (isset($field->field_info) && $field->field_info['type'] == 'geofield') {
          $this->add_geocluster_group_by_settings($field);
        }
      }
    };
  }

  /**
   * Perform clustering on the aggregated views result set.
   */
  function post_execute() {
    geophp_load();
    $results_by_geohash = $this->preClusterByGeohash();
    if (module_exists("devel")) {
      dd("pre-created clusters: " . timer_read("geocluster"));
    }

    $this->clusterByNeighborCheck($results_by_geohash);
    if (module_exists("devel")) {
      dd("merged & finalized clusters: " . timer_read("geocluster"));
    }

    $this->finalizeClusters();
    if (module_exists("devel")) {
      timer_stop("geocluster");
    }
  }

  /* HELPERS */

  protected function preClusterByGeohash() {
    $total_count = 0;
    $results_by_geohash = array();
    $group_field = $this->get_cluster_field_alias();
    foreach ($this->values as $row => &$value) {
      $hash_prefix = $value->$group_field;
      if (empty($hash_prefix)) {
        continue;
      }
      $total_count += $this->initCluster($value);
      $results_by_geohash[$hash_prefix] = array(
        $row => $value,
      );
    }

    if (module_exists("devel")) {
      dd("total items: " . $total_count);
    }
    return $results_by_geohash;
  }

  /**
   * Create final clusters by checking for overlapping neighbors.
   *
   * @param $results_by_geohash
   */
  protected function clusterByNeighborCheck(&$results_by_geohash) {
    foreach ($results_by_geohash as $current_hash => &$results) {
      if (empty($current_hash)) {
        continue;
      }
      $item_key = current(array_keys($results));
      $item = $results[$item_key];
      // Check top right neighbor hashes for overlapping points.
      // Top-right is enough because by the way geohash is structured,
      // future geohashes are always top, topright or right
      $hash_stack = GeohashHelper::getTopRightNeighbors($current_hash);
      foreach ($hash_stack as $hash) {
        if (isset($results_by_geohash[$hash])) {
          $other_item_key = current(array_keys($results_by_geohash[$hash]));
          $other_item = $results_by_geohash[$hash][$other_item_key];
          if ($this->shouldCluster($this->values[$item_key], $this->values[$other_item_key])) {
            $this->addCluster($item_key, $other_item_key, $current_hash, $hash, $results_by_geohash);
            if (!isset($results_by_geohash[$current_hash])) {
              continue 2;
            }
          }
        }
      }
    }
  }

  protected function finalizeClusters() {
    foreach ($this->values as &$value) {
      if ($value->geocluster_count > 1) {
        $value->clustered = TRUE;
      }
    }
  }

  /*** ALGORITHM HELPERS ***/

  protected function initCluster(&$value) {
    $lon = $value->geocluster_lon;
    $lat = $value->geocluster_lat;
    $value->geocluster_geometry = new Point($lon, $lat);
    $value->clustered = TRUE;
    return $value->geocluster_count;
  }

  /**
   * Determine if two geofields should be clustered as of their distance.
   */
  protected function shouldCluster($value, $value2) {
    // Calculate distance.
    $distance = GeoclusterHelper::distance_pixels($value->geocluster_geometry, $value2->geocluster_geometry, $this->resolution);
    return $distance <= $this->cluster_distance;
  }

  protected function get_cluster_field_alias() {
    $name = $this->get_cluster_field_name();
    return $this->field_handler->aliases[$name];
  }

  protected function get_cluster_field_name() {
    return current($this->field_handler->group_fields);
  }

  /**
   * Cluster two given rows.
   *
   * @param $row_id the first row to be clustered
   * @param $row_id2 the second row to be clustered
   */
  protected function addCluster($row_id, $row_id2, $hash, $hash2, &$entities_by_geohash) {
    $result1 = &$this->values[$row_id]; $result2 = &$this->values[$row_id2];

    // Merge cluster data.
    $result1->geocluster_ids .= ',' . $result2->geocluster_ids;
    $result1->geocluster_count += $result2->geocluster_count;

    // Calculate new center from all points.
    $center = GeoclusterHelper::getCenter(array($result1, $result2), array($result1->geocluster_count, $result2->geocluster_count));
    $result1->geocluster_geometry = $center;

    // Remove other result data that has been merged into the cluster.
    unset($this->values[$row_id2]);
    unset($entities_by_geohash[$hash2][$row_id2]);
    if (count($entities_by_geohash[$hash2]) == 0) {
      unset($entities_by_geohash[$hash2]);
    }
  }

  /** HELPERS */

  /**
   * @param $field views_handler_field
   */
  protected function add_geocluster_group_by_settings(&$field) {
    $group_column = 'geocluster_index_' . $this->getGeohashLength();
    $field->options['group_column'] = $group_column;

    // Add additional fields.
    // Add concatenated list of grouped entity ids.
    $this->add_fields(array('geocluster_ids' => 'entity_id'), 'group_concat', $field);

    // Add count(entity_id).
    $this->add_fields(array('geocluster_count' => 'entity_id'), 'count', $field);

    // Add center point: avg(lat), avg(lng).
    $avg_fields = array(
      'geocluster_lat' => $field->field . '_' . 'lat',
      'geocluster_lon' => $field->field . '_' . 'lon',
    );
    $this->add_fields($avg_fields, 'avg', $field);

    // Add select fields required for field formatter.
    // @todo: Remove? We don't support clustered data to be used with field formatters at the moment.
    // $this->add_fields(array($field->field . '_' . 'geom'), 'min', $field);
  }

  protected function add_fields($additional_fields, $function = NULL, &$field) {
    foreach ($additional_fields as $field_key => $additional_field) {
      $params = array();
      if (!empty($function)) {
        $params['function'] = $function;
      }
      $view = &$this->config->get_view();
      $view->query->add_field($field->table, $additional_field, $field_key, $params);
    }
  }
}


