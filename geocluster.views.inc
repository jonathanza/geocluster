<?php

/**
 * @file
 * Hooks for Views integration.
 */

/**
 * Implements hook_views_plugins().
 *
 */
function geocluster_views_plugins() {
  $plugins = array();
  $plugins['display_extender']['geocluster'] = array(
    'title' => t('Geocluster'),
    'help' => t('Configure geocluster settings.'),
    'handler' => 'GeoclusterViewsDisplayExtender',
    // 'uses options' => TRUE,
  );
  return $plugins;
}

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
 * Invokes clustering after the views query has been executed.
 *
 * Implements hook_views_post_execute_query.
 */
function geocluster_views_post_execute_query(&$view) {

  foreach ($view->field as $key => $my_field) {
    if ($my_field->definition['handler'] == 'geocluster_handler_field_geofield') {
      $field = $my_field;
    }
  }
  if (!isset($field)) {
    return;
  }

  $cluster_distance = $field->options['cluster_distance'];

  // @todo: Implement bbox logic and calculate zoom from bbox.

  // Default zoom
  $zoom = 7;
  switch ($view->style_plugin->plugin_name) {
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

  // Allow to override using get paramters for debugging purposes.
  $zoom = isset($_GET['zoom']) ? $_GET['zoom'] : $zoom;
  $cluster_distance = isset($_GET['cluster_distance']) ? $_GET['cluster_distance'] : $cluster_distance;
  $cluster = new Geocluster($cluster_distance, $zoom, $field);
  $cluster->cluster($view);
}

/**
 * Adds geocluster information to a leaflet data item.
 *
 * Implements hook_leaflet_views_alter_points_data.
 */
function geocluster_leaflet_views_alter_points_data($result, &$points) {
  if (isset($result->clustered)) {
    // Add cluster info to every points data entry.
    array_walk($points, function(&$point, $key, $count) {
      $point['clustered'] = TRUE;
      $point['cluster_items'] = $count;
    }, count($result->ids));
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
