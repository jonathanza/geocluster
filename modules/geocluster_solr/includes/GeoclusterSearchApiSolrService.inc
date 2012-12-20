<?php

/**
 * Search service class using Solr server.
 */
class GeoclusterSearchApiSolrService extends SearchApiSolrService {

  /*
   * @var Geocluster
   */
  var $geocluster;

  var $group_field;

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
   * Apply custom changes before the query
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
    // Get geocluster from query option, as we can't access the view object.
    // @see GeoclusterSearchApiViewsQuery::build().
    $geocluster = $query->getOption("geocluster");
    if ($geocluster != NULL) {
      $this->addGeoclusterParams($call_args, $query, $geocluster);
    }
  }

  protected function addGeoclusterParams(array &$call_args, SearchApiQueryInterface $query, Geocluster &$geocluster) {
    $this->geocluster = &$geocluster;

    $this->group_field = $geocluster->getFieldHandler()->field . ':geocluster_index_' . $geocluster->getGeohashLength();
    $group_limit = 1;

    // Solr params.
    $params = array(
      'group'           => 'true',
      'group.limit'     => $group_limit,
      'group.field'     => array('ss_' . $this->group_field),
    );
    $call_args['params'] += $params;
    //$call_args['limit'] = 10;
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
      $grouped_field = $fields[$this->group_field];
      $groups = &$response->grouped->{$grouped_field}->groups;
      foreach ($groups as $key => &$group) {
        if ($group->doclist->numFound > 1) {
          // Clustered results.
          // Set geocluster specific result data.
          $geohash = array_pop(explode('_', $group->groupValue));
          $location = geoPHP::load($geohash, 'geohash');
          $count = $group->doclist->numFound;
          $center = geofield_get_values_from_geometry($location);

          $entity = entity_create('geocluster_item', array(
            'field_geocluster_clustered' => TRUE,
            'field_geocluster_count' => $count,
            'field_geocluster_center' => $center,
          ));
        }
        else {
          // Single results.
          $doc = $group->doclist->docs[0];
          $id = $doc->{$fields['search_api_item_id']};

          $real_item = $id;
          $entity = entity_create('geocluster_item', array(
            'field_geocluster_clustered' => FALSE,
            'field_geocluster_real_item' => $real_item,
          ));
        }
        $results['results'][$id]['entity'] = $entity;
      }
    }
  }

  public function getFieldNames(SearchApiIndex $index, $reset = FALSE) {
    $field_names = parent::getFieldNames($index, $reset);
    // $field_names[$this->group_field] = array();
    return $field_names;
  }

  }
