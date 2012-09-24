<?php

/**
 * @file
 * Geocluster extension for geofield and maps.
 */

class Geocluster {

  /**
   * @todo: don't rely on a custom field handler,
   * use another type of views plugin instead.
   * we might want to make views_object_types() pluggable?
   *
   * @var Geofield (geocluster) field handler on which to perform clustering.
   */
  var $field_handler;

  /**
   * @var current zoom resolution for clustering in meters / pixel.
   *
   * @see GeoclusterHelper::resultions().
   */
  var $resolution;

  var $zoom_level;

  var $cluster_distance;

  function __construct($cluster_distance, $zoom, $field_handler) {
    $this->cluster_distance = $cluster_distance;
    $this->zoom_level = $zoom;
    $resolutions = GeoclusterHelper::resolutions();
    $this->resolution = $resolutions[$zoom];
    $this->field_handler = $field_handler;
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

    $entities_by_type = $this->entities_by_type($view->result);
    $entities_by_geohash = $this->load_entity_fields($entities_by_type, $view->result);

    $values = &$view->result;

    if(TRUE) {
      // Geohash-based sort algorithm.
      $this->geohashSort($entities_by_geohash, $values);
    }
    else {
      // Simple sort algorithm.
      $this->simpleSort($values);
    }



  }

  function geohashSort($entities_by_geohash, &$values) {
    foreach ($entities_by_geohash as $current_hash => &$entities) {
      $cluster = NULL;

      // Add all points within the core geohash to a cluster.
      foreach ($entities_by_geohash[$current_hash] as $key => &$entity) {

        if (!isset($cluster)) {
          $cluster = $entity;
          $cluster_id = $key;
        }
        else {
          $this->addCluster($values, $cluster_id, $key);
          unset($entities_by_geohash[$current_hash][$key]);
          if (count($entities_by_geohash[$current_hash]) == 0) {
            unset($entities_by_geohash[$current_hash]);
          }
        }
      }

      // Check neighbor hashes for overlapping points.
      $hash_stack = GeohashHelper::getNeighbors($current_hash);
      foreach ($hash_stack as $hash) {
        if (isset($entities_by_geohash[$hash])) {
          foreach ($entities_by_geohash[$hash] as $key => &$entity) {
            if ($this->shouldCluster($values, $values[$cluster_id], $values[$key])) {
              $this->addCluster($values, $cluster_id, $key);
              unset($entities_by_geohash[$hash][$key]);
              if (count($entities_by_geohash[$hash]) == 0) {
                unset($entities_by_geohash[$hash]);
              }
            }
          }
        }
      }
    }
  }

  function simpleSort(&$values) {
    // Perform clustering.
    for($i = 0; $i < count($values); $i++) {
      $value = &$values[$i];

      for($j = $i+1; $j < count($values); $j++) {
        $value2 = &$values[$j];

        if ($this->shouldCluster($values, $value, $value2)) {
          $this->addCluster(&$values, $i, $j);
        }
      }
    }
  }

  /**
   * Cluster two given rows.
   *
   * @param $values all result rows
   * @param $row_id the first row to be clustered
   * @param $row_id2 the second row to be clustered
   */
  protected function addCluster(&$values, $row_id, $row_id2) {
    $result1 = &$values[$row_id];
    $result2 = &$values[$row_id2];

    $field_alias = $this->field_handler->field_alias;

    $cluster = &$result1;
    // @todo: make a real cluster, currently it still links to the first result.
    if (!isset($cluster->clustered)) {
      $cluster->node_title = 'Cluster: ' . $result1->node_title;
      $cluster->ids = array(
        $result1->{$field_alias}
      );
      $cluster->clustered = TRUE;
    }
    $cluster->node_title .= ' + ' . $result2->node_title;
    $cluster->ids += array($result2->{$field_alias});

    $geofield = &$this->get_geofield_with_geometry($result1, $field_alias);
    $geofield2 = &$this->get_geofield_with_geometry($result2, $field_alias);
    $center = new MultiPoint(array($geofield['geometry'], $geofield2['geometry']));

    $geofield = geofield_get_values_from_geometry($center->centroid());
    unset($values[$row_id2]);
  }

  /**
   * Determine if two geofields should be clustered as of their distance.
   */
  protected function shouldCluster($values, $value, $value2) {

    $field_alias = $this->field_handler->field_alias;

    // Load geofields.
    $geofield = $this->get_geofield_with_geometry($value, $field_alias);
    $geofield2 = $this->get_geofield_with_geometry($value2, $field_alias);

    // Calculate distance.
    $distance = GeoclusterHelper::distance_pixels($geofield['geometry'], $geofield2['geometry'], $this->resolution);
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

  function load_entity_fields($entities_by_type, &$values) {
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
      $geohash_length = GeohashHelper::lengthFromDistance($this->cluster_distance, $this->resolution);

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
    asort($entities_by_geohash);
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
}
