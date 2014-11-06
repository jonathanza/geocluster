<?php

/**
 * Search service class using Solr server.
 */
class GeoclusterSearchApiViewsQuery extends SearchApiViewsQuery {

  public function init($base_table, $base_field, $options) {
    parent::init($base_table, $base_field, $options);
  }

  public function build(&$view) {
    parent::build($view);

    if ($geocluster = geocluster_get_algorithm($view)) {
      $this->query->setOption("geocluster", $geocluster);
    }
  }

  /**
   * Helper function for adding results to a view in the format expected by the
   * view.
   */
  protected function addResults(array $results, $view) {
    // Let the original handler process the results first.
    parent::addResults($results, $view);

    // Do postprocessing if necessary.
    if ($geocluster = geocluster_get_algorithm($view)) {
      foreach ($view->result as $views_result) {
        $entity_id = $views_result->entity;
        // Add the related solr geocluster data to the views result. Those data
        // can be used with the geocluster views fields.
        if (isset($results[$entity_id])) {
          $views_result->clustered = $results[$entity_id]['clustered'];
          $views_result->geocluster_ids = $results[$entity_id]['geocluster_ids'];
          $views_result->geocluster_count = $results[$entity_id]['geocluster_count'];
          $views_result->geocluster_geometry = $results[$entity_id]['geocluster_geometry'];
          $views_result->geocluster_lat = $results[$entity_id]['geocluster_lat'];
          $views_result->geocluster_lon = $results[$entity_id]['geocluster_lon'];
        }
      }
    }
  }

  protected function prepareEntityRow(&$row, $result, $id, &$missing, &$items, &$ids) {
    // Include the loaded item for this result row, if present, or the item
    // ID.
    if (!empty($result['entity'])) {
      $row['entity'] = $result['entity'];
    }
    else {
      $row['entity'] = $id;
    }

    $row['_entity_properties']['search_api_relevance'] = $result['score'];
    $row['_entity_properties']['search_api_excerpt'] = empty($result['excerpt']) ? '' : $result['excerpt'];

    // Gather any fields from the search results.
    if (!empty($result['fields'])) {
      $row['_entity_properties'] += $result['fields'];
    }

    // Check whether we need to extract any properties from the result item.
    $missing_fields = array_diff_key($this->fields, $row);
    if ($missing_fields) {
      $missing[$id] = $missing_fields;
      if (is_object($row['entity'])) {
        $items[$id] = $row['entity'];
      }
      else {
        $ids[] = $id;
      }
    }
  }

}
