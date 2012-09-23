<?php

/**
 * @file
 * Geocluster field implementation.
 *
 * Based on Geofield, see geocluster_views_data_alter.
 */

class geocluster_handler_field_geofield extends views_handler_field_field {

  function option_definition() {
    $options = parent::option_definition();
    $more_options['cluster_distance'] = array(
      'default' => 15,
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

  /**
   * Perform clustering on post_execute for the field handler.
   *
   * Runs after the view is executed, before the result is cached.
   *
   * @param $values all result rows
   */
  function post_execute(&$values) {
    return parent::post_execute($values);

    $style_options = $this->view->display_handler->display->display_options['style_options'];
    $zoom = $style_options['geofield_map_zoom'];

    $geocluster = new Geocluster((int)$zoom);

    if (!empty($values)) {
      // Load entities as the standard views_field_handler does it.
      $this->load_entities($values);

      $geocluster->cluster($values);

      // Now, transfer the data back into the resultset so it can be easily used.
      foreach ($values as $row_id => &$value) {
        $value->{'field_' . $this->options['id']} = $this->set_items($value, $row_id);
      }
    }
  }

  function get_value($values, $field = NULL) {
    return parent::get_value($values, $field);
  }

  /* ### more ### */

  /**
   * @todo: this duplicates existing views functionality, file a patch?
   * see views_handler_field_field::post_execute()
   */
  protected function load_entities(&$values) {
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

    $entities = array();
    foreach ($entities_by_type as $entity_type => $entity_ids) {
      $entity_info = entity_get_info($entity_type);
      if (empty($this->definition['is revision'])) {
        $entities = entity_load($entity_type, $entity_ids);
        $keys = $entity_ids;
      }
      else {
        // Revisions can't be loaded multiple, so we have to load them
        // one by one.
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
