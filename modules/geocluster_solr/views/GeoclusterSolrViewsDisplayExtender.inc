<?php


class GeoclusterSolrViewsDisplayExtender extends views_plugin_display_extender {

  function option_definition() {
    $options = parent::option_definition();
    $more_options['cluster_distance'] = array(
      'default' => 22,
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
