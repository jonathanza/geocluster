<?php

/**
 * Search service class using Solr server.
 */
class GeoclusterSearchApiSolrService extends SearchApiSolrService {

  /*
   * @var Geocluster
   */
  var $geocluster;

  var $cluster_field;
  var $group_field;
  var $geohash_field;
  var $latlon_field;
  var $id_field;

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
    $field_clusters = "clusters";
    $field_docs = "docs";
    $field_center = "locs_center";
    $field_doc_ids = "sm_doc_ids";
    $field_count = "is_count";
    $field_id = "ss_id";

    $index = $query->getIndex();
    $fields = $this->getFieldNames($index);
    $field_options = $index->options['fields'];

    // Set up the results array.
    $results = array();
    $results['results'] = array();
    // In some rare cases (e.g., MLT query with nonexistent ID) the response
    // will be NULL.
    if (!isset($response->clusters)) { // This is named grouped instead of response
      $results['result count'] = 0;
      return $results;
    }

    $total_count = 0;
    $clusters = $response->{$field_clusters};
    $docs = $clusters->{$field_docs};

    // Add each search result to the results array.
    foreach ($docs as $cluster) {

      if ($cluster->{$field_count}) {
        $count = $cluster->{$field_count};
        $latlon = explode(",", $cluster->{$field_center});
        $doc_ids = $cluster->{$field_doc_ids};
        $id = $doc_ids[0];

        $total_count += $count;
        $result = array(
          'id' => $id,
          'score' => NULL,
        );

        // mock entity properties for SearchApiViewsQuery::addResults()
        // $row['_entity_properties'] += $result['fields'];
        $result['fields'] = array(
          // Cluster info.
          'cluster' => TRUE,
          'cluster_count' => $count,
          'cluster_items' => $doc_ids,

          // Fields.
          'nid' => $id,
          'field_place:lat' => $latlon[0],
          'field_place:lon' => $latlon[1],
        );

        // Use the result's id as the array key. By default, 'id' is mapped to
        // 'item_id' in SearchApiSolrService::getFieldNames().
        if ($result['id']) {
          $results['results'][$result['id']] = $result;
        }
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
    $geocluster = $query->getOption("geocluster");
    if ($geocluster != NULL) {
      $this->addGeoclusterParams($call_args, $query, $geocluster);
    }
  }

  protected function addGeoclusterParams(array &$call_args, SearchApiQueryInterface $query, Geocluster &$geocluster) {
    $this->geocluster = &$geocluster;

    $this->cluster_field = $geocluster->getFieldHandler()->field;
    $this->group_field = 'f_ss_' . $this->cluster_field . ':geohash_geocluster_index_' . $geocluster->getGeohashLength();
    $this->geohash_field = 'geohashs_' . $this->cluster_field . ':geohash';
    $this->latlon_field = 'locs_' . $this->cluster_field . ':latlon';
    $this->id_field = 'ss_search_api_id';

    // Geocluster specific params.
    $geocluster_params = array(

      // Geocluster params.
      'geocluster.clusterDistance' => $geocluster->getClusterDistance(),
      'geocluster.zoomLevel' => $geocluster->getZoomLevel(),
      'geocluster.resolution' => $geocluster->getResolution(),
      'geocluster.geohashLength' => $geocluster->getGeohashLength(),

      // Geocluster solr params.
      'geocluster.clusterField' => $this->cluster_field,
      'geocluster.groupField' => $this->group_field,
      'geocluster.geohashField' => $this->geohash_field,
      'geocluster.latlonField' => $this->latlon_field,
      'geocluster.idField' => $this->id_field,
    );

    $group_limit = 5;

    // Solr params.
    $params = array(
      'qt'              => array('/geocluster'),
      'query'           => array(),
      'fl'              => array('*'),
      'fq'              => array('index_id:geocluster_index'),
      'facet.field'     => array($this->group_field),
      'group'           => array('true'),
      'group.limit'     => array($group_limit),
      'group.field'     => array($this->group_field),
      'facet'           => array('true'),
      'facet.prefix'    => array($geocluster->getGeohashLength() . '_'),
    );
    $call_args['params'] += $params + $geocluster_params;
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
