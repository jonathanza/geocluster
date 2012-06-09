<?php

/**
 * @file
 * Distance field implementation.
 */

class geocluster_handler_field_geofield extends views_handler_field_field {

  function option_definition() {
    $options = parent::option_definition();
    $more_options['cluster_distance'] = array(
      'default' => 10,
    );
    $options = $more_options + $options;
    return $options;
  }

  function options_form(&$form, &$form_state) {
    parent::options_form($form, $form_state);
    $more_form['cluster_distance'] = array(
      '#type' => 'textfield',
      '#title' => t('Cluster distance'),
      '#default_value' => $this->options['cluster_distance'],
      '#description' => t('Specify the cluster distance.'),
    );
    $form = $more_form + $form;
  }

  function options_validate(&$form, &$form_state) {
  }

  function post_execute(&$values) {
    geophp_load();
    $i = 1;
    if (!empty($values)) {
      $entities_by_type = $this->get_entities_by_type($values);
      $entities = $this->load_entities($values, $entities_by_type);

      foreach ($values as $row_id => &$value) {
        $geofield = $this->get_geofield_with_geometry($entities, $value);
        foreach ($values as $row_id2 => &$value2) {
          if ($row_id == $row_id2) {
            continue;
          }
          $geofield2 = $this->get_geofield_with_geometry($entities, $value2);
          if ($this->shouldCluster($geofield, $geofield2)) {
            $this->addCluster(&$values, $row_id, $row_id2);
          }
        }
      }

      // Now, transfer the data back into the resultset so it can be easily used.
      foreach ($values as $row_id => &$value) {
        $value->{'field_' . $this->options['id']} = $this->set_items($value, $row_id);
      }
    }
  }

  protected function addCluster(&$values, $row_id, $row_id2) {
    $result1 = $values[$row_id];
    $result2 = $values[$row_id2];

    $cluster = $result1;
    // @todo: make a real cluster, currently it still links to the first result.
    if (!isset($cluster->clustered)) {
      $cluster->node_title = 'Cluster: ' . $result1->node_title;
      $cluster->ids = array(
        $result1->{$this->field_alias}
      );
      $cluster->clustered = TRUE;
    }
    $cluster->node_title .= ' + ' . $result2->node_title;
    $cluster->ids += array($result2->{$this->field_alias});

    // @todo: calculate and set cluster center
    unset($values[$row_id2]);
  }

  protected function shouldCluster($geofield, $geofield2) {
    $distance = geocluster_distance($geofield['geometry'], $geofield2['geometry']);
    return $distance <= $this->options['cluster_distance'];
  }

  protected function get_geofield_with_geometry($entities, $value) {
     $entity_id = $value->{$this->field_alias};
     $geofield = $entities[$entity_id]->{$this->field_info['field_name']}[LANGUAGE_NONE][0];
     if (!isset($geofield['geometry'])) {
       $geofield['geometry'] = geoPHP::load($geofield['wkt'], 'wkt');
     }
     return $geofield;
  }


  /**
   * see views_handler_field_field::post_execute()
   */
  protected function get_entities_by_type($values) {
    // Divide the entity ids by entity type, so they can be loaded in bulk.
    $entities_by_type = array();
    $revisions_by_type = array();
    foreach ($values as $key => $object) {
      if (isset($object->{$this->field_alias}) && !isset($values[$key]->_field_data[$this->field_alias])) {
        $entity_type = $object->{$this->aliases['entity_type']};
        if (empty($this->definition['is revision'])) {
          $entity_id = $object->{$this->field_alias};
          $entities_by_type[$entity_type][$key] = $entity_id;
        }
        else {
          $revision_id = $object->{$this->field_alias};
          $entity_id = $object->{$this->aliases['entity_id']};
          $entities_by_type[$entity_type][$key] = array($entity_id, $revision_id);
        }
      }
    }

    return $entities_by_type;
  }

  /**
   * see views_handler_field_field::post_execute()
   */
  protected function load_entities(&$values, $entities_by_type) {
    foreach ($entities_by_type as $entity_type => $entity_ids) {
      $entity_info = entity_get_info($entity_type);
      if (empty($this->definition['is revision'])) {
        $entities = entity_load($entity_type, $entity_ids);
        $keys = $entity_ids;
      }
      else {
        // Revisions can't be loaded multiple, so we have to load them
        // one by one.
        $entities = array();
        $keys = array();
        foreach ($entity_ids as $key => $combined) {
          list($entity_id, $revision_id) = $combined;
          $entity = entity_load($entity_type, array($entity_id), array($entity_info['entity keys']['revision'] => $revision_id));
          if ($entity) {
            $entities[$revision_id] = array_shift($entity);
            $keys[$key] = $revision_id;
          }
        }
      }

      foreach ($keys as $key => $entity_id) {
        // If this is a revision, load the revision instead.
        if (isset($entities[$entity_id])) {
          $values[$key]->_field_data[$this->field_alias] = array(
            'entity_type' => $entity_type,
            'entity' => $entities[$entity_id],
          );
        }
      }
    }
    return $entities;
  }

}
