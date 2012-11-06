<?php
/**
 * @file
 * geocluster_solr_demo.views_default.inc
 */

/**
 * Implements hook_views_default_views().
 */
function geocluster_solr_demo_views_default_views() {
  $export = array();

  $view = new view();
  $view->name = 'search_api_geocluster';
  $view->description = '';
  $view->tag = 'default';
  $view->base_table = 'search_api_index_geocluster_index';
  $view->human_name = 'search api geocluster';
  $view->core = 7;
  $view->api_version = '3.0';
  $view->disabled = FALSE; /* Edit this to true to make a default view disabled initially */

  /* Display: Master */
  $handler = $view->new_display('default', 'Master', 'default');
  $handler->display->display_options['title'] = 'search api geocluster';
  $handler->display->display_options['use_more_always'] = FALSE;
  $handler->display->display_options['access']['type'] = 'none';
  $handler->display->display_options['cache']['type'] = 'none';
  $handler->display->display_options['query']['type'] = 'views_query';
  $handler->display->display_options['exposed_form']['type'] = 'basic';
  $handler->display->display_options['pager']['type'] = 'none';
  $handler->display->display_options['style_plugin'] = 'views_geojson';
  $handler->display->display_options['style_options']['data_source']['value'] = 'latlon';
  $handler->display->display_options['style_options']['data_source']['latitude'] = 'field_place_lat';
  $handler->display->display_options['style_options']['data_source']['longitude'] = 'field_place_lon';
  $handler->display->display_options['style_options']['data_source']['geofield'] = 'field_place';
  $handler->display->display_options['style_options']['data_source']['wkt'] = 'nid';
  $handler->display->display_options['style_options']['data_source']['name_field'] = 'nid';
  $handler->display->display_options['style_options']['jsonp_prefix'] = '';
  $handler->display->display_options['style_options']['using_views_api_mode'] = 0;
  /* Field: Indexed Node: Node ID */
  $handler->display->display_options['fields']['nid']['id'] = 'nid';
  $handler->display->display_options['fields']['nid']['table'] = 'search_api_index_geocluster_index';
  $handler->display->display_options['fields']['nid']['field'] = 'nid';
  /* Field: Indexed Node: Place » Latitude */
  $handler->display->display_options['fields']['field_place_lat']['id'] = 'field_place_lat';
  $handler->display->display_options['fields']['field_place_lat']['table'] = 'search_api_index_geocluster_index';
  $handler->display->display_options['fields']['field_place_lat']['field'] = 'field_place_lat';
  $handler->display->display_options['fields']['field_place_lat']['precision'] = '0';
  $handler->display->display_options['fields']['field_place_lat']['link_to_entity'] = 0;
  /* Field: Indexed Node: Place » Longitude */
  $handler->display->display_options['fields']['field_place_lon']['id'] = 'field_place_lon';
  $handler->display->display_options['fields']['field_place_lon']['table'] = 'search_api_index_geocluster_index';
  $handler->display->display_options['fields']['field_place_lon']['field'] = 'field_place_lon';
  $handler->display->display_options['fields']['field_place_lon']['precision'] = '0';
  $handler->display->display_options['fields']['field_place_lon']['link_to_entity'] = 0;
  /* Field: Indexed Node: Place */
  $handler->display->display_options['fields']['field_place']['id'] = 'field_place';
  $handler->display->display_options['fields']['field_place']['table'] = 'search_api_index_geocluster_index';
  $handler->display->display_options['fields']['field_place']['field'] = 'field_place';
  $handler->display->display_options['fields']['field_place']['label'] = '';
  $handler->display->display_options['fields']['field_place']['element_label_colon'] = FALSE;
  $handler->display->display_options['fields']['field_place']['click_sort_column'] = 'geom';
  $handler->display->display_options['fields']['field_place']['type'] = 'geofield_latlon';
  $handler->display->display_options['fields']['field_place']['settings'] = array(
    'data' => 'full',
    'format' => 'decimal_degrees',
    'labels' => 1,
  );
  /* Contextual filter: Custom: Bounding box */
  $handler->display->display_options['arguments']['bbox_argument']['id'] = 'bbox_argument';
  $handler->display->display_options['arguments']['bbox_argument']['table'] = 'search_api_index_geocluster_index';
  $handler->display->display_options['arguments']['bbox_argument']['field'] = 'bbox_argument';
  $handler->display->display_options['arguments']['bbox_argument']['default_action'] = 'default';
  $handler->display->display_options['arguments']['bbox_argument']['default_argument_type'] = 'querystring';
  $handler->display->display_options['arguments']['bbox_argument']['summary']['number_of_records'] = '0';
  $handler->display->display_options['arguments']['bbox_argument']['summary']['format'] = 'default_summary';
  $handler->display->display_options['arguments']['bbox_argument']['summary_options']['items_per_page'] = '25';
  $handler->display->display_options['geocluster_enabled'] = 0;
  $handler->display->display_options['geocluster_options']['cluster_field'] = 'field_place';
  $handler->display->display_options['geocluster_options']['cluster_distance'] = '';

  /* Display: Page */
  $handler = $view->new_display('page', 'Page', 'page');
  $handler->display->display_options['defaults']['hide_admin_links'] = FALSE;
  $handler->display->display_options['geocluster_enabled'] = 1;
  $handler->display->display_options['geocluster_options']['cluster_field'] = 'field_place';
  $handler->display->display_options['geocluster_options']['cluster_distance'] = '33';
  $handler->display->display_options['path'] = 'search-api-geocluster';
  $export['search_api_geocluster'] = $view;

  return $export;
}
