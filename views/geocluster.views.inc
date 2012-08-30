<?php

/**
 * @file
 * Hooks for Views integration.
 */

/**
 * Register a geocluster field handler for every geofield.
 *
 * Implements hook_views_data_alter().
 */
function geocluster_views_data_alter(&$data) {
  // @todo: use views_data hook?
  // @todo: load geofield names dynamically.
  $geofields = array(
    'field_data_field_place' => 'field_place'
  );
  foreach ($data as $table_name => $table_data) {
    if (isset($geofields[$table_name])) {
      $field_name = $geofields[$table_name];
      // Custom geocluster field handler settings.
      $data[$table_name]['field_geocluster_geofield'] = array(
        'group' => 'Content',
        'title'       => $table_data[$field_name]['title'] . ' - clustered',
        'title short' => $table_data[$field_name]['title'] . ' - clustered',
        'help' => $table_data[$field_name]['help'],
        'field' => array(
          'field' => 'field_geocluster_geofield',
          'table' => $table_name,
          'handler' => 'geocluster_handler_field_geofield',
          'field_name' => $field_name,
          'real_field' => $table_name,
        ),
      );
      // Merge custom geocluster field handler settings with default ones.
      $data[$table_name]['field_geocluster_geofield']['field'] = array_merge(
        $data[$table_name][$field_name]['field'],
        $data[$table_name]['field_geocluster_geofield']['field']
      );
    }
  }
}

/**
 * Implements hook_views_plugins().
 *
 * Adds GeoJSON feed style.
 */
/* NOT USED AT THE MOMENT
function geocluster_views_plugins() {
  $path = drupal_get_path('module', 'geocluster');

  $plugins = array(
    'module' => 'geocluster',
    'style' => array(),
  );

  $plugins['style']['geocluster_geofield_map'] = array(
    'title' => t('Geocluster Geofield Map'),
    'help' => t('Displays a View as clustered Geofield map.'),
    'handler' => 'geocluster_plugin_style_geofield_map',
    'theme' => 'geofield_map_map',
    'theme path' => $path . '/includes',
    'path' => $path . '/includes',
    'uses fields' => TRUE,
    'uses row plugin' => FALSE,
    'uses options' => TRUE,
    'uses grouping' => FALSE,
    'type' => 'normal',
    'even empty' => TRUE,
  );

  $plugins['style']['geocluster_geojson'] = array(
    'title'            => t('Geocluster GeoJSON Feed'),
    'help'             => t('Displays clustered nodes in the GeoJSON data format.'),
    'handler'          => 'geocluster_plugin_style_geojson',
    //'theme'            => 'views_views_geojson_style',
    //'theme path'       => $path . '/views',
    'path'             => $path . '/views',
    'uses row plugin'  => FALSE,
    'uses fields'      => TRUE,
    'uses options'     => TRUE,
    //'uses grouping'    => FALSE,
    //'type'             => 'feed',
    'type'             => 'normal',
    //'help_topic'     => 'style-geojson',
    'even empty'     => TRUE,
  );
  return $plugins;
}
*/
