<?php


class GeoclusterViewsDisplayExtender extends views_plugin_display_extender {

  function options_definition_alter(&$options) {
    // options_definition() doesn't work for display_extender plugins.
    // see http://drupal.org/node/681468#comment-4384814
    // and http://drupal.org/node/1616540
    $options['geocluster_enabled']['default'] = FALSE;
    $options['geocluster_options'] = array(
      'contains' => array(
        'cluster_distance' => array('default' => 12),
      ),
    );
  }

  function options_form(&$form, &$form_state) {
    // Add a checkbox to enable clustering.
    $more_form['geocluster_enabled'] = array(
      '#type' => 'checkbox',
      '#title' => 'Enable geocluster for this search.',
      '#default_value' => $this->display->get_option('geocluster_enabled'),
      '#description' => t("@todo: description"),
    );

    // An additional fieldset provides additional options.
    $geocluster_options = $this->display->get_option('geocluster_options');
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
    $more_form['geocluster_options']['cluster_distance'] = array(
      '#type' => 'textfield',
      '#title' => t('Cluster distance'),
      '#default_value' => $geocluster_options['cluster_distance'],
      '#description' => t('Specify the cluster distance.'),
    );

    $form = $more_form + $form;
  }

  function options_validate(&$form, &$form_state) {
  }

  function options_submit(&$form, &$form_state) {
    $this->display->set_option('geocluster_enabled', $form_state['values']['geocluster_enabled']);
    $this->display->set_option('geocluster_options', $form_state['values']['geocluster_options']);
  }

}
