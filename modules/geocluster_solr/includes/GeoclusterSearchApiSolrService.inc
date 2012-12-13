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
    if ($this->geocluster) {
      // Set grouping options, so search api will process single result groups correctly.
      $options = array(
        'use_grouping' => TRUE,
        'fields' => array($this->group_field),
      );
      $query->setOption('search_api_grouping', $options);
    }

    // Let search api retrieve the results as usual.
    $results = parent::extractResults($query, $response);
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
    $this->group_field = $this->cluster_field . ':geocluster_index_' . $geocluster->getGeohashLength();
    $this->geohash_field = 'geohashs_' . $this->cluster_field . ':geohash';
    $this->latlon_field = 'locs_' . $this->cluster_field . ':latlon';
    $this->id_field = 'ss_search_api_id';

    $group_limit = 1;

    // Solr params.
    $params = array(
      'group'           => array('true'),
      'group.limit'     => array($group_limit),
      'group.field'     => array('ss_' . $this->group_field),
    );
    $call_args['params'] += $params;
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
    // Post-process clustered results.
    if ($this->geocluster) {
      geophp_load();
      $index = $query->getIndex();
      $fields = $this->getFieldNames($index);
      $groups = &$response->grouped->{'ss_' . $this->group_field}->groups;
      foreach ($groups as $key => &$group) {
        // Clustered results.
        if ($group->doclist->numFound > 1) {
          $doc = $group->doclist->docs[0];
          $id = $doc->{$fields['search_api_item_id']};
          $result = &$results['results'][$id];
          // Set geocluster specific result data.
          $result['clustered'] = TRUE;
          $result['geocluster_count'] = $group->doclist->numFound;
          $geohash = array_pop(explode('_', $group->groupValue));
          // TODO: improve accuracy?
          $location = geoPHP::load($geohash, 'geohash');
          $result['geocluster_lat'] = $location->getY();
          $result['geocluster_lon'] = $location->getX();
          // $result['geocluster_ids'] = "1,2,3";
        }
      }
    }
    $i = 1;
  }

  public function getFieldNames(SearchApiIndex $index, $reset = FALSE) {
    $field_names = parent::getFieldNames($index, $reset);
    // $field_names[$this->group_field] = array();
    return $field_names;
  }

  }
