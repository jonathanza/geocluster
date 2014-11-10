<?php

define('GEOCLUSTER_DEFAULT_DISTANCE', 65);
define('GEOCLUSTER_DEFAULT_ALGORITHM', 'geocluster_mysql');


/**
 * Encapsulates the geocluster config.
 */
class GeoclusterConfig implements GeoclusterConfigBackendInterface {

  /**
   * @var GeoclusterConfigBackendInterface
   */
  var $config_backend;

  function __construct($config_backend) {
    $this->config_backend = $config_backend;
  }

  function options_definition() {
    $options = array();
    $options['geocluster_enabled']['default'] = FALSE;
    $options['geocluster_options'] = array();
    return $options;
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
        // '#description' => t("@todo: description"),
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
      $algorithm_options = $this->get_algorithm_options();
      $more_form['geocluster_options']['algorithm'] = array(
        '#type' => 'select',
        '#title' => t('Clustering algorithm'),
        '#description' => t('Select a geocluster algorithm to be used.'),
        '#options' => $algorithm_options,
        '#default_value' => $geocluster_options['algorithm'] ? $geocluster_options['algorithm'] : GEOCLUSTER_DEFAULT_ALGORITHM,
      );
      $more_form['geocluster_options']['cluster_field'] = array(
        '#type' => 'select',
        '#title' => t('Cluster field'),
        '#description' => t('Select the geofield to be used for clustering.?'),
        '#options' => $cluster_field_options,
        '#default_value' => $geocluster_options['cluster_field'] ? $geocluster_options['cluster_field'] : '',
      );
      $more_form['geocluster_options']['cluster_distance'] = array(
        '#type' => 'textfield',
        '#title' => t('Cluster distance'),
        '#default_value' => $geocluster_options['cluster_distance'] ? $geocluster_options['cluster_distance'] : GEOCLUSTER_DEFAULT_DISTANCE,
        '#description' => t('Specify the cluster distance.'),
      );
      $more_form['geocluster_options']['enable_bbox_support'] = array(
        '#type' => 'checkbox',
        '#title' => t('Enable bbox support'),
        '#default_value' => !empty($geocluster_options['enable_bbox_support']),
        '#description' => t('If enabled available Views GeoJSON bbox support will be enhanced.'),
      );
    }
    $form = $more_form + $form;
  }

  function options_validate(&$form, &$form_state) {
  }

  function options_submit(&$form, &$form_state) {
    $this->set_option('geocluster_enabled', $form_state['values']['geocluster_enabled']);
    $this->set_option('geocluster_options', $form_state['values']['geocluster_options']);

    $geocluster_options = $form_state['values']['geocluster_options'];
    if ($geocluster_options['algorithm'] == 'geocluster_mysql') {
      if (!$this->get_option('group_by')) {
        $this->set_option('group_by', TRUE);
        drupal_set_message(t('The <strong>use aggregation</strong> setting has been <em>enabled</em> as a requirement by the MySQL-based geocluster algorithm.'));
      }
    }
    else if ($geocluster_options['algorithm'] == 'geocluster_php') {
      if ($this->get_option('group_by')) {
        $this->set_option('group_by', FALSE);
        drupal_set_message(t('The <strong>use aggregation</strong> setting has been <em>disabled</em> as a requirement by the PHP-based geocluster algorithm.'));
      }
    }
  }

  function get_cluster_field_options() {
    // Inspired by geofield.
    $handlers = $this->get_display()->get_handlers('field');
    $cluster_field_options = array(
      '' => '<none>',
    );
    foreach ($handlers as $handler) {
      $field_info = NULL;
      if (!empty($handler->field_info)) {
        $field_info = $handler->field_info;
      }
      elseif ($this->is_entity_views_handler($handler)) {
        // Strip the basic field name from the entity views handler field and
        // fetch the field info for it.
        $property = EntityFieldHandlerHelper::get_selector_field_name($handler->real_field);
        if ($field_name = EntityFieldHandlerHelper::get_selector_field_name(substr($handler->real_field, 0, strpos($handler->real_field, ':' . $property)), ':')) {
          $field_info = field_info_field($field_name);
        }
      }
      if (!empty($field_info['type']) && $field_info['type'] == 'geofield') {
        $cluster_field_options[$handler->options['id']] = (!empty($handler->options['label'])) ? $handler->options['label'] : $handler->options['id'];
      }
    }
    return $cluster_field_options;
  }

  /**
   * Provide a list of available geocluster algorithm options.
   * @return array
   */
  protected function get_algorithm_options() {
    $algorithms = ctools_get_plugins('geocluster', 'algorithm');
    $options = array();
    foreach ($algorithms as $id => $algorithm) {
      $options[$id] = $algorithm['handler']['label'];
    }
    return $options;
  }

  /**
   * Returns a configuration option value.
   */
  public function get_option($option) {
    return $this->config_backend->get_option($option);
  }

  /**
   * Sets an option value.
   */
  public function set_option($option, $value) {
    $this->config_backend->set_option($option, $value);
  }

  /**
   * Returns the view that the configuration is attached to.
   * @return View
   */
  public function get_view() {
    return $this->config_backend->get_view();
  }

  /**
   * Returns the display of the configuration.
   * @return views_plugin_display
   */
  public function get_display() {
    return $this->config_backend->get_display();
  }

  /**
   * Checks if a field handler class is handled by the entity module.
   *
   * @param object $field_handler_instance
   *   The field handler instance to check.
   *
   * @return bool
   *   TRUE if the field is handled by the entity module views integration.
   */
  public function is_entity_views_handler($field_handler_instance) {
    $static_cache = &drupal_static(__METHOD__, array());
    $handler_class = get_class($field_handler_instance);

    if (!isset($static_cache[$handler_class])) {
      $static_cache[$handler_class] = FALSE;
      foreach (entity_views_get_field_handlers() as $field_handler) {
        if ($field_handler_instance instanceof $field_handler) {
          return $static_cache[$handler_class] = TRUE;
        }
      }
    }
    return $static_cache[$handler_class];
  }
}
