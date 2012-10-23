<?php


/**
 * Implements hook_views_plugins().
 *
 */
function geocluster_solr_views_plugins() {
  $plugins = array();
  $plugins['display_extender']['geocluster'] = array(
    'title' => t('Geocluster Solr Display Extender'),
    'help' => t('Configure geocluster solr settings.'),
    'handler'          => 'GeoclusterSolrViewsDisplayExtender',
    'uses options' => TRUE,
  );
  return $plugins;
}
