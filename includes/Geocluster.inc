<?php

/**
 * @file
 * Geocluster extension for geofield and maps.
 */

class Geocluster {

  /**
   * Reference to Geofield field handler on which to perform clustering.
   *
   * @var views_handler_field
   */
  var $field_handler;

  /**
   * Minimum cluster distance, as defined by GeoclusterViewsDisplayExtender.
   * @todo: document unit.
   *
   * @var float
   */
  var $cluster_distance;

  /**
   * Current zoom level for clustering.
   *
   * @var int
   */
  var $zoom_level;

  /**
   * Resolution in meters / pixel based on zoom_level.
   * @see GeoclusterHelper::resultions().
   *
   * @var float
   */
  var $resolution;

  /**
   * Geohash length for clustering by a specified distance in pixels.
   *
   * @var int
   */
  var $geohash_length;

  /**
   * Constructor.
   *
   * @param $cluster_distance
   * @param $zoom
   * @param $field_handler views_handler_field
   */
  function __construct($cluster_distance, $zoom, $field_handler) {
    $this->field_handler = $field_handler;
    $this->cluster_distance = $cluster_distance;
    $this->zoom_level = $zoom;
    $resolutions = GeoclusterHelper::resolutions();
    $this->resolution = $resolutions[$zoom];
    $this->geohash_length = GeohashHelper::lengthFromDistance($this->cluster_distance, $this->resolution);

    if (module_exists("devel")) {
      $debug =
        "zoom: " . $this->zoom_level .
          " , resolution: " . $this->resolution .
          " , distance: " . $this->cluster_distance .
          " , geohash_length: " . $this->geohash_length;
      dd("");
      dd($debug);
    }
  }

  /*** MAIN ALGORITHM ***/

  /**
   * Perform clustering on a given views result set.
   *
   * This should be called from a hook_views_post_execute_query() implementation.
   *
   * @param $view View containing the result set to cluster.
   */
  public function cluster(&$view) {
    geophp_load();
    $values = &$view->result;

    if (module_exists("devel")) {
      $query =  Database::getConnection()->prefixTables(vsprintf($view->build_info['query'], $view->build_info['query_args']));
      $replacements = module_invoke_all('views_query_substitutions', $view);
      $query = str_replace(array_keys($replacements), $replacements, $query);
      dd($query);

      timer_start("geocluster");
      dd("started clustering " . count($values) . " items");
    }

    $results_by_geohash = $this->preClusterByGeohash($values);
    if (module_exists("devel")) {
      dd("pre-created clusters: " . timer_read("geocluster"));
    }

    $this->clusterByNeighborCheck($values, $results_by_geohash);
    if (module_exists("devel")) {
      dd("merged & finalized clusters: " . timer_read("geocluster"));
    }

    $this->finalizeClusters($values);
    if (module_exists("devel")) {
      timer_stop("geocluster");
    }
  }

  protected function preClusterByGeohash(&$values) {
    $total_count = 0;
    $results_by_geohash = array();
    $group_field = $this->get_cluster_field_alias();
    foreach ($values as $row => &$value) {
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
  protected function clusterByNeighborCheck(&$values, &$results_by_geohash) {
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
          if ($this->shouldCluster($values[$item_key], $values[$other_item_key])) {
            $this->addCluster($values, $item_key, $other_item_key, $current_hash, $hash, $results_by_geohash);
            if (!isset($results_by_geohash[$current_hash])) {
              continue 2;
            }
          }
        }
      }
    }
  }

  protected function finalizeClusters(&$values) {
    foreach ($values as &$value) {
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
   * @param $values all result rows
   * @param $row_id the first row to be clustered
   * @param $row_id2 the second row to be clustered
   */
  protected function addCluster(&$values, $row_id, $row_id2, $hash, $hash2, &$entities_by_geohash) {
    $result1 = &$values[$row_id]; $result2 = &$values[$row_id2];

    // Merge cluster data.
    $result1->geocluster_ids .= ',' . $result2->geocluster_ids;
    $result1->geocluster_count += $result2->geocluster_count;

    // Calculate new center from all points.
    $center = GeoclusterHelper::getCenter(array($result1, $result2), array($result1->geocluster_count, $result2->geocluster_count));
    $result1->geocluster_geometry = $center;

    // Remove other result data that has been merged into the cluster.
    unset($values[$row_id2]);
    unset($entities_by_geohash[$hash2][$row_id2]);
    if (count($entities_by_geohash[$hash2]) == 0) {
      unset($entities_by_geohash[$hash2]);
    }
  }

  /*** GETTERS & SETTERS ***/

  /**
   * @return float
   */
  public function getClusterDistance() {
    return $this->cluster_distance;
  }

  /**
   * @return \views_handler_field
   */
  public function getFieldHandler() {
    return $this->field_handler;
  }

  /**
   * @return float
   */
  public function getResolution() {
    return $this->resolution;
  }

  /**
   * @return int
   */
  public function getZoomLevel() {
    return $this->zoom_level;
  }

  /**
   * @return int
   */
  public function getGeohashLength() {
    return $this->geohash_length;
  }

}
