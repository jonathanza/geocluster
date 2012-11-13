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

  function __construct($cluster_distance, $zoom, $field_handler) {
    $this->field_handler = $field_handler;
    $this->cluster_distance = $cluster_distance;
    $this->zoom_level = $zoom;
    $resolutions = GeoclusterHelper::resolutions();
    $this->resolution = $resolutions[$zoom];
    $this->geohash_length = GeohashHelper::lengthFromDistance($this->cluster_distance, $this->resolution);

    // @todo: Remove debug code.
    $debug =
        "zoom: " . $this->zoom_level .
        " , resolution: " . $this->resolution .
        " , distance: " . $this->cluster_distance .
        " , geohash_length: " . $this->geohash_length;
    // print($debug);
    dd($debug);
  }

  function get_cluster_field_alias() {
    $name = $this->get_cluster_field_name();
    return $this->field_handler->aliases[$name];
  }

  function get_cluster_field_name() {
    return current($this->field_handler->group_fields);
  }

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

    dd("started clustering " . count($values) . " items");
    timer_start("geocluster");

    // Geohash-based sort algorithm.
    $this->geohashCluster($values);
  }

  function geohashCluster(&$values) {
    $use_database_cluster = TRUE;
    if ($use_database_cluster) {
      $group_field = $this->get_cluster_field_alias();
      foreach ($values as $row => $value) {
        $hash_prefix = $value->$group_field;
        if (empty($hash_prefix)) {
          continue;
        }

        $lon = $value->geocluster_lon;
        $lat = $value->geocluster_lat;
        $value->geocluster_geometry = new Point($lon, $lat);

        $results_by_geohash[$hash_prefix] = array(
          $row => $value,
        );
      }
    }
    else {
      $this->clusterByHash($values);
    }

    dd("prepared entities: " . timer_read("geocluster"));

    dd("created clusters: " . timer_read("geocluster"));

    // Loop over geohash-based pre-clusters to create real clusters.
    // Check top right neighbor hashes for overlapping points.
    // Top-right is enough because by the way geohash is structured,
    // future geohashes are always top, topright or right
    foreach ($results_by_geohash as $current_hash => &$results) {
      if (empty($current_hash)) {
        continue;
      }
      $item_key = current(array_keys($results));
      $item = $results[$item_key];
      $hash_stack = GeohashHelper::getTopRightNeighbors($current_hash);
      foreach ($hash_stack as $hash) {
        if (isset($results_by_geohash[$hash])) {
          $other_item_key = current(array_keys($results_by_geohash[$hash]));
          $other_item = $results_by_geohash[$hash][$other_item_key];
          if ($this->shouldCluster($values, $values[$item_key], $values[$other_item_key])) {
            $this->addCluster($values, $item_key, $other_item_key, $current_hash, $hash, $results_by_geohash, TRUE);
            if (!isset($results_by_geohash[$current_hash])) {
              continue 2;
            }
          }
        }
      }
      $this->finalizeCluster($values, $item_key);
    }

    dd("merged & finalized clusters: " . timer_read("geocluster"));
    timer_stop("geocluster");
  }

  protected function clusterByHash(&$values) {
    // Prepare input data & parameters.
    $entities_by_type = $this->entities_by_type($values);
    // Generate geohash-based pre-clusters.
    $results_by_geohash = $this->load_entity_fields($entities_by_type, $values, $this->geohash_length);

    // Loop over geohash-based pre-clusters to create real clusters.
    foreach ($results_by_geohash as $current_hash => &$entities) {
      $cluster_id = NULL;
      // Add all points within the current geohash to a cluster.
      foreach ($entities as $key => &$entity) {
        if (!isset($cluster_id)) {
          $cluster_id = $key;
        }
        else {
          $this->addCluster($values, $cluster_id, $key, $current_hash, $current_hash, $results_by_geohash);
        }
      }
      $this->updateCluster($values, $cluster_id);
    }
  }

  /**
   * Cluster two given rows.
   *
   * @param $values all result rows
   * @param $row_id the first row to be clustered
   * @param $row_id2 the second row to be clustered
   */
  protected function addCluster(&$values, $row_id, $row_id2, $hash, $hash2, &$entities_by_geohash, $update_center = FALSE) {
    $result1 = &$values[$row_id];
    $result2 = &$values[$row_id2];
    $field_alias = $this->field_handler->field_alias;

    if (isset($result1->clustered) || !isset($result2->clustered)) {
      $cluster = &$result1;
      $other = &$result2;
      $other_id = $row_id2;
      $other_hash = $hash2;
    }
    else {
      $cluster = &$result2;
      $other = &$result1;
      $other_id = $row_id;
      $other_hash = $hash;
    }

    // @todo: make a real cluster.
    if (!isset($cluster->clustered)) {
      $cluster->cluster_items = array($cluster);
      $cluster->clustered = TRUE;
    }

    if (!isset($other->clustered)) {
      $cluster->cluster_items[] = $other;
    }
    else {
      $cluster->cluster_items = array_merge($cluster->cluster_items, $other->cluster_items);
    }

    if ($update_center) {
      // Calculate new center from all points.
      $center = $this->getCenter(array($cluster, $other));

      $cluster->geocluster_geometry = $center;

      // php version
      // Overwrite cluster geofield with center value.
      // $geofield = geofield_get_values_from_geometry($center->centroid());
      // $this->set_geofield($cluster, $geofield);
    }

    unset($values[$other_id]);
    unset($entities_by_geohash[$other_hash][$other_id]);

    if (count($entities_by_geohash[$other_hash]) == 0) {
      unset($entities_by_geohash[$other_hash]);
    }
  }

  protected function updateCluster(&$values, $cluster_id) {
    $cluster = &$values[$cluster_id];
    if (isset($cluster->clustered)) {
      $field_alias = $this->field_handler->field_alias;

      // Calculate new center from all points.
      $center = $this->getCenter($cluster->cluster_items);

      // Overwrite cluster geofield with center value.
      $geofield = geofield_get_values_from_geometry($center->centroid());
      $this->set_geofield($cluster, $geofield);
    }
  }

  protected function getCenter($items) {
    foreach ($items as $item) {
      $lat = $item->geocluster_geometry->getY();
      $lon = $item->geocluster_geometry->getX();
      $lat_min = isset($lat_min) ? min($lat_min, $lat) : $lat;
      $lat_max = isset($lat_max) ? max($lat_max, $lat) : $lat;
      $lon_min = isset($lon_min) ? min($lon_min, $lon) : $lon;
      $lon_max = isset($lon_max) ? max($lon_max, $lon) : $lon;
    }
    $lat = ($lat_max + $lat_min) / 2;
    $lon = ($lon_max + $lon_min) / 2;
    $center = new Point($lon, $lat);
    return $center;
  }

  protected function finalizeCluster(&$values, $cluster_id) {
    $cluster = &$values[$cluster_id];
    if (isset($cluster->clustered)) {
      $field_alias = $this->field_handler->field_alias;
      $cluster->ids = array();
      $cluster->node_title = 'Cluster ';
      foreach ($cluster->cluster_items as $cluster_item) {
        $cluster->node_title .= '- ' . $cluster_item->node_title;
        $cluster->ids[] = $cluster_item->{$field_alias};
      }
      // Avoid cluster being treated as a normal item.
      if (isset($cluster->{$field_alias})) {
        unset($cluster->{$field_alias});
      }
    }
    $geofield = geofield_get_values_from_geometry($cluster->geocluster_geometry);
    $this->set_geofield($cluster, $geofield);
  }

  /**
   * Determine if two geofields should be clustered as of their distance.
   */
  protected function shouldCluster($values, $value, $value2) {
    $field_alias = $this->field_handler->field_alias;

    // Load geofields.
    $geometry = $value->geocluster_geometry;
    $geometry2 = $value2->geocluster_geometry;

    // Calculate distance.
    $distance = GeoclusterHelper::distance_pixels($geometry, $geometry2, $this->resolution);
    return $distance <= $this->cluster_distance;
  }

  /**
   * see views_handler_field_field::post_execute()
   */
  function entities_by_type(&$values) {
    // Divide the entity ids by entity type, so they can be loaded in bulk.
    $entities_by_type = array();
    $revisions_by_type = array();
    foreach ($values as $key => $object) {
      if (isset($object->{$this->field_handler->field_alias}) && !isset($values[$key]->_field_data[$this->field_handler->field_alias])) {
        $entity_type = $object->{$this->field_handler->aliases['entity_type']};
        if (empty($this->field_handler->definition['is revision'])) {
          $entity_id = $object->{$this->field_handler->field_alias};
          $entities_by_type[$entity_type][$key] = $entity_id;
        }
        else {
          $revision_id = $object->{$this->field_handler->field_alias};
          $entity_id = $object->{$this->field_handler->aliases['entity_id']};
          $entities_by_type[$entity_type][$key] = array($entity_id, $revision_id);
        }
      }
    }
    return $entities_by_type;
  }

  function load_entity_fields($entities_by_type, &$values, $geohash_length) {
    // Load only the field data required for geoclustering.
    // This saves us unnecessary entity loads.
    foreach ($entities_by_type as $entity_type => $my_entities) {
      // Use EFQ for preparing entities to be used in field_attach_load().
      $query = new EntityFieldQuery();
      $query->entityCondition('entity_type', 'node');
      $query->entityCondition('entity_id', $my_entities, 'IN');
      $result = $query->execute();
      $entities = $result[$entity_type];
      field_attach_load(
        $entity_type,
        $entities,
        FIELD_LOAD_CURRENT,
        array('field_id' => $this->field_handler->field_info['id'])
      );
      // @todo handle revisions?

      $keys = $my_entities;
      $entities_by_geohash = array();

      foreach ($keys as $key => $entity_id) {
        // If this is a revision, load the revision instead.
        if (isset($entities[$entity_id])) {
          $values[$key]->_field_data[$this->field_handler->field_alias] = array(
            'entity_type' => $entity_type,
            'entity' => $entities[$entity_id],
          );

          $geofield = $this->get_geofield_with_geometry($values[$key]);
          $geohash_key = substr($geofield['geohash'], 0, $geohash_length);
          if (!isset($entities_by_geohash[$geohash_key])) {
            $entities_by_geohash[$geohash_key] = array();
          }
          $entities_by_geohash[$geohash_key][$key] = $entities[$entity_id];
        }
      }
    }
    ksort($entities_by_geohash);
    return $entities_by_geohash;
  }

  /**
   * Helper function to get the geofield with its geometry for a given result.
   *
   * Geometry will only we loaded once and stored in the geofield.
   *
   * @param $entities all result entities that have been loaded
   * @param $value the current result row value set
   */
  function &get_geofield_with_geometry(&$value) {
    $entity = &$value->_field_data[$this->field_handler->field_alias]['entity'];

    $geofield = &$entity->{$this->field_handler->field_info['field_name']}[LANGUAGE_NONE][0];
    if (!isset($geofield['geometry'])) {
      $geofield['geometry'] = geoPHP::load($geofield['geom'], 'wkb');
    }
    if (!isset($geofield['geohash'])) {
      $geofield['geohash'] = $geofield['geometry']->out('geohash');
    }
    return $geofield;
  }

  function set_geofield(&$value, &$geofield) {
    $data = &$value->geocluster_center;

    // $data = &$value->_field_data[$this->field_handler->field_alias];
    $data['entity']->{$this->field_handler->field_info['field_name']}[LANGUAGE_NONE][0] = $geofield;
    /*
    if (!isset($data['entity_type'])) {
      $data['entity_type'] = "node";
    }
    $data['entity']['type'] = 'article';
    */
  }

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
