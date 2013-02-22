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
    'label' => 'Geocluster test mysql json map',
    'description' => NULL,
    'title' => '',
    'type' => 'leaflet_geojson_bean',
    'data' => array(
      'view_mode' => 'default',
      'settings' => array(
        'source' => 'geocluster_test_mysql_json',
        'map' => 'mapbox streets',
        'height' => '400',
        'override_map_settings' => 1,
        'map_settings' => array(
          'zoom' => '2',
          'center' => array(
            'lon' => '14.56667',
            'lat' => '48.03333',
          ),
        ),
      ),
    ),
    'delta' => 'geocluster-test-mysql-json-map',
    'view_mode' => 'default',
    'uuid' => '5fd0c588-70ed-fc94-61b8-188e05450406',
    'settings' => array(
      'source' => 'geocluster_test_mysql_json',
      'map' => 'mapbox streets',
      'height' => '400',
      'override_map_settings' => 1,
      'map_settings' => array(
        'zoom' => '2',
        'center' => array(
          'lon' => '14.56667',
          'lat' => '48.03333',
        ),
      ),
    ),
  );
  $beans[] = array(
    'label' => 'Geocluster test php json map',
    'description' => NULL,
    'title' => '',
    'type' => 'leaflet_geojson_bean',
    'data' => array(
      'view_mode' => 'default',
      'settings' => array(
        'source' => 'geocluster_test_php_json',
        'map' => 'mapbox streets',
        'height' => '400',
        'override_map_settings' => 1,
        'map_settings' => array(
          'zoom' => '2',
          'center' => array(
            'lon' => '14.56667',
            'lat' => '48.03333',
          ),
        ),
      ),
    ),
    'delta' => 'geocluster-test-php-json-map',
    'view_mode' => 'default',
    'uuid' => 'e7767fe6-3b15-4b9c-b24e-b9fdf9ada963',
    'settings' => array(
      'source' => 'geocluster_test_php_json',
      'map' => 'mapbox streets',
      'height' => '400',
      'override_map_settings' => 1,
      'map_settings' => array(
        'zoom' => '2',
        'center' => array(
          'lon' => '14.56667',
          'lat' => '48.03333',
        ),
      ),
    ),
  );
  return $beans;
}
