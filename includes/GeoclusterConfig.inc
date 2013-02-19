<?php

define('GEOCLUSTER_DEFAULT_DISTANCE', 12);

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
    $options['geocluster_options'] = array(
      'contains' => array(
        'cluster_field' => array('default' => ''),
        'cluster_distance' => array('default' => GEOCLUSTER_DEFAULT_DISTANCE),
      ),
    );
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
      $algorithm_options = $this->get_algorithm_options();
      $more_form['geocluster_options']['algorithm'] = array(
        '#type' => 'select',
        '#title' => t('Clustering algorithm'),
        '#description' => t('Select a geocluster algorithm to be used.'),
        '#options' => $algorithm_options,
        '#default_value' => $geocluster_options['algorithm'] ? $geocluster_options['algorithm'] : '',
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
        '#default_value' => $geocluster_options['cluster_distance'],
        '#description' => t('Specify the cluster distance.'),
      );
    }
    $form = $more_form + $form;
  }

  function options_submit(&$form, &$form_state) {
    $this->set_option('geocluster_enabled', $form_state['values']['geocluster_enabled']);
    $this->set_option('geocluster_options', $form_state['values']['geocluster_options']);
  }

  function get_cluster_field_options() {
    // Inspired by geofield.
    $handlers = $this->get_display()->get_handlers('field');
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

  /**
   * Provide a list of available geocluster algorithm options.
   * @return array
   */
  protected function get_algorithm_options() {
    $algorithms = geocluster_get_geocluster_algorithm_info(NULL, TRUE);
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
}
