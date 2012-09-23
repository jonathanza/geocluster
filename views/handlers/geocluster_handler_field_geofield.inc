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
}
