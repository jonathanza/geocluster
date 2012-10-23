<?php

/**
 * Search service class using Solr server.
 */
class GeoclusterSearchApiSolrService extends SearchApiSolrService {


  /**
   * Extract results from a Solr response.
   *
   * @param Apache_Solr_Response $response
   *   A response object from SolrPhpClient.
   *
   * @return array
   *   An array with two keys:
   *   - result count: The number of total results.
   *   - results: An array of search results, as specified by
   *     SearchApiQueryInterface::execute().
   */
  protected function extractResults(SearchApiQueryInterface $query, Apache_Solr_Response $response) {
    $index = $query->getIndex();
    $fields = $this->getFieldNames($index);
    $field_options = $index->options['fields'];

    // Set up the results array.
    $results = array();
    $results['results'] = array();
    // In some rare cases (e.g., MLT query with nonexistent ID) the response
    // will be NULL.
    if (!isset($response->grouped)) { // This is named grouped instead of response
      $results['result count'] = 0;
      return $results;
    }

    // @todo: dynamically determine field.
    $field_name = "f_ss_field_place:geohash_geocluster_index_3";

    $total_count = 0;
    $groups = $response->grouped->{$field_name}->groups;

    // Add each search result to the results array.
    foreach ($groups as $group) {

      $total_count += $group->doclist->numFound;
      $result = array(
        'cluster' => TRUE,
        'cluster_count' => $group->doclist->numFound,
        'cluster_items' => $group->doclist->docs,
        'id' => $group->groupValue,
        'score' => NULL,
      );

      // mock entity properties for SearchApiViewsQuery::addResults()
      // $row['_entity_properties'] += $result['fields'];
      $latlon_field = "t_field_place:latlon";
      $latlon = explode(",", $result['cluster_items'][0]->{$latlon_field}[0]);
      $result['fields'] = array(
        'nid' => $result['cluster_items'][0]->item_id,
        'field_place:lat' => $latlon[0],
        'field_place:lon' => $latlon[1],
      );

      // Use the result's id as the array key. By default, 'id' is mapped to
      // 'item_id' in SearchApiSolrService::getFieldNames().
      if ($result['id']) {
        $results['results'][$result['id']] = $result;
      }

    }

    $results['result count'] = $total_count;
    return $results;
  }

  /**
   * Empty method to allow subclassed to apply custom changes before the query
   * is sent to Solr. Works exactly like hook_search_api_solr_query_alter().
   *
   * @param array $call_args
   *   An associative array containing all four arguments to the
   *   Apache_Solr_Service::search() call ("query", "offset", "limit" and
   *   "params") as references.
   * @param SearchApiQueryInterface $query
   *   The SearchApiQueryInterface object representing the executed search query.
   */
  protected function preQuery(array &$call_args, SearchApiQueryInterface $query) {
    // @todo: parametrize & optimize currently hardcoded query params.
    $params = array(
      'qt'              => array('/geocluster'),
      'query'           => array(),
      'fl'              => array('*'),
      'fq'              => array('index_id:geocluster_index'),
      'facet.field'     => array('f_ss_field_place:geohash_geocluster_index_3'),
      'group'           => array('true'),
      'group.limit'     => array('5'),
      'group.field'     => array('f_ss_field_place:geohash_geocluster_index_3'),
      'facet'           => array('true'),
      'facet.prefix'    => array('3_'),
    );
    $call_args['params'] += $params;
    $call_args['query'] = '*:*';
  }

  /**
   * Empty method to allow subclasses to apply custom changes before search results are returned.
   *
   * Works exactly like hook_search_api_solr_search_results_alter().
   *
   * @param array $results
   *   The results array that will be returned for the search.
   * @param SearchApiQueryInterface $query
   *   The SearchApiQueryInterface object representing the executed search query.
   * @param Apache_Solr_Response $response
   *   The response object returned by Solr.
   */
  protected function postQuery(array &$results, SearchApiQueryInterface $query, Apache_Solr_Response $response) {
    $i = 2;
  }

}
