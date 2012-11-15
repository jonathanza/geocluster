<?php

/**
 * Diameter of the Earth in kilometers.
 */
define('GEOCLUSTER_DEFAULT_DISTANCE', 12);

class GeoclusterViewsDisplayExtender extends views_plugin_display_extender {

  /**
   * @var Geocluster
   */
  var $geocluster;

  function options_definition_alter(&$options) {
    // options_definition() doesn't work for display_extender plugins.
    // see http://drupal.org/node/681468#comment-4384814
    // and http://drupal.org/node/1616540
    $options['geocluster_enabled']['default'] = FALSE;
    $options['geocluster_options'] = array(
      'contains' => array(
        'cluster_field' => array('default' => ''),
        'cluster_distance' => array('default' => GEOCLUSTER_DEFAULT_DISTANCE),
      ),
    );
  }

  function options_form(&$form, &$form_state) {
    $cluster_field_options = $this->get_cluster_field_options();
    if (count($cluster_field_options) == 1) {
      $more_form['error'] = array(
        '#markup' => 'Please add at least 1 geofield to the view',
      );
    }
    else {
      // Add a checkbox to enable clustering.
      $more_form['geocluster_enabled'] = array(
        '#type' => 'checkbox',
        '#title' => 'Enable geocluster for this search.',
        '#default_value' => $this->get_option('geocluster_enabled'),
        '#description' => t("@todo: description"),
      );

      // An additional fieldset provides additional options.
      $geocluster_options = $this->get_option('geocluster_options');
      $more_form['geocluster_options'] = array(
        '#type' => 'fieldset',
        '#title' => 'Geocluster options',
        '#tree' => TRUE,
        '#states' => array(
          'visible' => array(
            ':input[name="geocluster_enabled"]' => array('checked' => TRUE),
          ),
        ),
      );
      $more_form['geocluster_options']['cluster_field'] = array(
        '#type' => 'select',
        '#title' => t('Cluster field'),
        '#description' => t('Which field contains the geodata for clustering?'),
        '#options' => $cluster_field_options,
        '#default_value' => $geocluster_options['cluster_field'] ? $geocluster_options['cluster_field'] : '',
      );
      $more_form['geocluster_options']['cluster_distance'] = array(
        '#type' => 'textfield',
        '#title' => t('Cluster distance'),
        '#default_value' => $geocluster_options['cluster_distance'],
        '#description' => t('Specify the cluster distance.'),
      );

      }

    $form = $more_form + $form;
  }

  function options_validate(&$form, &$form_state) {
  }

  function options_submit(&$form, &$form_state) {
    $this->display->set_option('geocluster_enabled', $form_state['values']['geocluster_enabled']);
    $this->display->set_option('geocluster_options', $form_state['values']['geocluster_options']);
  }

  function get_cluster_field_options() {
    // Inspired by geofield.
    $handlers = $this->display->get_handlers('field');
    $cluster_field_options = array(
      '' => '<none>',
    );
    foreach ($handlers as $handle) {
      if (!empty($handle->field_info['type']) && $handle->field_info['type'] == 'geofield') {
        $cluster_field_options[$handle->options['id']] = (!empty($handle->options['label'])) ? $handle->options['label'] : $handle->options['id'];
      }
    }
    return $cluster_field_options;
  }

  function pre_execute() {
    if ($this->get_option('geocluster_enabled')) {
      if ($this->geocluster = $this->init_geocluster()) {

      }

      if($this->get_option('group_by')) {
        foreach ($this->view->field as $field_key => $field) {
          if (isset($field->field_info) && $field->field_info['type'] == 'geofield') {
            $this->add_geocluster_group_by_settings($field);
          }
        }
      };
    }
  }

  function init_geocluster() {
    $view = $this->view;

    // Get geocluster options.
    $geocluster_options = $this->get_option('geocluster_options');
    $cluster_distance = $geocluster_options['cluster_distance'];
    $field_name = $geocluster_options['cluster_field'];
    if (empty($view->field[$field_name])) {
      return;
    }
    $cluster_field = $view->field[$field_name];

    // Default zoom.
    $zoom = 7;

    // Add customizations based on style plugin.
    // @todo: make this a separate function or hook
    $style_plugin_name = $this->get_option('style_plugin');
    switch ($style_plugin_name) {
      case 'leaflet':
        $map = $view->style_plugin->options['map'];
        $info =  leaflet_map_get_info($map);
        $zoom = $info['settings']['zoom'];
        break;
      case 'views_geojson':
        break;
      case 'geofield_map':
        $style_options = $view->display_handler->display->display_options['style_options'];
        $zoom = $style_options['geofield_map_zoom'];
    }

    // Allow to override using get paramters.
    $zoom = isset($_GET['zoom']) ? $_GET['zoom'] : $zoom;
    $cluster_distance = isset($_GET['cluster_distance']) ? $_GET['cluster_distance'] : $cluster_distance;

    // Create geocluster instance.
    $cluster = new Geocluster($cluster_distance, $zoom, $cluster_field);
    $this->set_option('geocluster_instance', $cluster);
    return $cluster;
  }

  /**
   * @param $field views_handler_field
   */
  function add_geocluster_group_by_settings(&$field) {
    $group_column = 'geocluster_index_' . $this->geocluster->getGeohashLength();
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

  function add_fields($additional_fields, $function = NULL, &$field) {
    foreach ($additional_fields as $field_key => $additional_field) {
      $params = array();
      if (!empty($function)) {
        $params['function'] = $function;
      }
      $this->view->query->add_field($field->table, $additional_field, $field_key, $params);
    }
  }

  /** HELPERS */

  function get_option($option) {
    return $this->display->get_option($option);
  }

  function set_option($option, $value) {
    return $this->display->set_option($option, $value);
  }

}


