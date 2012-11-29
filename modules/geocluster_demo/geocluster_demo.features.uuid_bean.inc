<?php
/**
 * @file
 * geocluster_demo.features.uuid_bean.inc
 */

/**
 * Implements hook_uuid_features_default_beans().
 */
function geocluster_demo_uuid_features_default_beans() {
  $beans = array();

  $beans[] = array(
    'label' => 'Geocluster test groupby json map',
    'description' => NULL,
    'title' => '',
    'type' => 'leaflet_geojson_bean',
    'data' => array(
      'view_mode' => 'default',
      'settings' => array(
        'source' => 'geocluster_test_groupby_page',
        'map' => 'mapbox streets',
        'height' => '400',
      ),
    ),
    'delta' => 'geocluster-test-groupby-json-map',
    'view_mode' => 'default',
    'uuid' => '5fd0c588-70ed-fc94-61b8-188e05450406',
    'settings' => array(
      'source' => 'geocluster_test_groupby_page',
      'map' => 'mapbox streets',
      'height' => '400',
    ),
  );
  return $beans;
}
