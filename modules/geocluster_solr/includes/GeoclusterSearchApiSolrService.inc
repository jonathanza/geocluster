<?php
/**
 * @file
 * Service class to enhance the existing Search API Solr service.
 */

/**
 * Search service class using Solr server.
 */
class GeoclusterSearchApiSolrService extends SearchApiSolrService {

  /**
   * The clustering algorithm.
   *
   * @see GeoclusterSearchApiSolrService::getAlgorithm()
   *
   * @var SolrGeohashGeoclusterAlgorithm
   */
  protected $algorithm;

  /**
   * The field to group on.
   *
   * @see GeoclusterSearchApiSolrService::getGroupingField()
   *
   * @var string|FALSE
   */
  protected $groupField;

  /**
   * Holds the bbox query if search_api_location isn't present.
   *
   * @see GeoclusterSearchApiSolrService::handleBboxFilter()
   *
   * @var string
   */
  protected $bboxQuery;

  /**
   * Stores the base name of the location field to use.
   *
   * @see GeoclusterSearchApiSolrService::getLocationFieldBaseName()
   *
   * @var string
   */
  protected $locationFieldBaseName;

  /**
   * Returns the configured algorithm object.
   *
   * @param SearchApiQueryInterface $query
   *   The query to use.
   *
   * @return SolrGeohashGeoclusterAlgorithm|FALSE
   *   The algorithm instance or FALSE if none is set.
   */
  public function &getAlgorithm(SearchApiQueryInterface $query) {
    if (!isset($this->algorithm)) {
      $this->algorithm = FALSE;
      // Get geocluster from query option, as we can't access the view object.
      // @see GeoclusterSearchApiViewsQuery::build().
      $geocluster = $query->getOption('geocluster');
      if ($geocluster != NULL) {
        $this->algorithm = &$geocluster;
      }
    }
    return $this->algorithm;
  }

  /**
   * Returns the base field name without the property selector.
   *
   * We don't know for sure what's the base name of the location field used in
   * the index. It's likely the base name contains a property extension e.g:
   * - base_name:latlon
   * - base_name:geom
   * - base_name:lat
   * - base_name:lon
   *
   * This method strips the property part.
   *
   * @param SearchApiQueryInterface $query
   *   The query to use.
   *
   * @return string
   *   The base name of the location field.
   */
  protected function getLocationFieldBaseName($query) {
    if (!isset($this->locationFieldBaseName)) {
      $field_handler = $this->getAlgorithm($query)->getFieldHandler();
      $field_name = $this->locationFieldBaseName = $field_handler->real_field;
      // Get the field property of the field used and adjust the field base name
      // if a property was found.
      if (($field_property = EntityFieldHandlerHelper::get_selector_field_name($field_name))) {
        $this->locationFieldBaseName = substr($field_handler->real_field, 0, strpos($field_name, ':' . $field_property));
      }
    }
    return $this->locationFieldBaseName;
  }

  /**
   * Get the grouping field.
   *
   * @param SearchApiQueryInterface $query
   *   The query to use.
   *
   * @return string|FALSE
   *   The field name to group by. Name can be used to declare the field in the
   *   query option search_api_grouping. Returns FALSE on failure.
   */
  public function getGroupingField(SearchApiQueryInterface $query) {
    if (!isset($this->groupField)) {
      $this->groupField = FALSE;

      $algorithm = $this->getAlgorithm($query);

      $index = $query->getIndex();
      $fields = $this->getFieldNames($index);
      $base_field_name = $this->getLocationFieldBaseName($query);

      // Determine field to group on.
      $group_property = ':geocluster_index_' . $algorithm->getGeohashLength();
      $group_field_name = $base_field_name . $group_property;
      if (!isset($fields[$group_field_name])) {
        watchdog('Geocluster', 'Unable to cluster results because the grouping field @field is not available. Please check if the field is enabled in your search index.', array('@field' => $group_field_name), WATCHDOG_ERROR, 'admin/config/search/search_api/index/' . $index->machine_name);
        return FALSE;
      }
      $this->groupField = $group_field_name;
    }
    return $this->groupField;
  }

  /**
   * Parses the combined bbox value into the single values.
   *
   * A bbox is defined as: left,bottom,right,top. This method splits the
   * combined values into it's parts and returns them as associative array.
   *
   * @TODO Add a way to configure the parameter name.
   *
   * @return array|FALSE
   *   Associative array with the bbox coordinates in the keys:
   *   - left
   *   - bottom
   *   - right
   *   - top
   */
  protected function getBboxArray() {
    if (isset($_GET['bbox'])) {
      $parts = explode(',', $_GET['bbox']);
      if (count($parts) == 4) {
        return array(
          'left' => (float) $parts[0],
          'bottom' => (float) $parts[1],
          'right' => (float) $parts[2],
          'top' => (float) $parts[3],
        );
      }
    }
    return FALSE;
  }

  /**
   * Handles the bbox filtering.
   *
   * If Search API Location is available the bbox query option is used,
   * otherwise the solr query string is prepared that is used here
   * @see GeoclusterSearchApiSolrService::addGeoclusterParams()
   *
   * @param SearchApiQueryInterface $query
   *   The query to use.
   *
   * @return string|FALSE
   *   FALSE if nothing is to handle, otherwise the filter string to use in
   *   GeoclusterSearchApiSolrService::addGeoclusterParams().
   */
  public function handleBboxFilter(SearchApiQueryInterface $query) {
    if (!isset($this->bboxQuery)) {
      $this->bboxQuery = FALSE;
      $algorithm = $this->getAlgorithm($query);

      $index = $query->getIndex();
      $fields = $this->getFieldNames($index);
      $field_handler = $algorithm->getFieldHandler();
      $field_name = $field_handler->real_field;

      // Add bbox filter, if present and valid.
      if (($bbox = $this->getBboxArray())) {

        // If Search API Location is present use it for bbox filtering.
        if (module_exists('search_api_location')) {
          $location_fields = search_api_location_get_location_fields($index);
          $search_api_location = $query->getOption('search_api_location', array());
          $search_api_location[] = array(
            'field' => key($location_fields),
            'bbox' => $bbox,
          );
          $query->setOption('search_api_location', $search_api_location);
        }
        else {
          // Without Search API Location we prepare the query which then is used
          // in GeoclusterSearchApiSolrService::addGeoclusterParams()
          $location_field_name = $field_name;

          // If the field isn't a location field try to fetch the related
          // location field.
          if (empty($field_handler->definition['type']) || $field_handler->definition['type'] != 'location') {
            $location_field_name = FALSE;
            // Get the base name of the field and see if the latlon property is
            // available as well.
            $base_field_name = $this->getLocationFieldBaseName($query);
            if (isset($fields[$base_field_name . ':latlon'])) {
              $location_field_name = $base_field_name . ':latlon';
            }
          }
          if ($location_field_name) {
            $field = $fields[$location_field_name];
            $field = SearchApiSolrConnection::escapeFieldName($field);
            $this->bboxQuery = $field . ':[' . $bbox['bottom'] . ',' . $bbox['left'] . ' TO ' . $bbox['top'] . ',' . $bbox['right'] . ']';
          }
        }
      }
    }
    return $this->bboxQuery;
  }

  /**
   * Modifies the solr query to include the clustering.
   *
   * @param array $call_args
   *   The arguments for the solr query.
   * @param SearchApiQueryInterface $query
   *   The query to process.
   */
  protected function addGeoclusterParams(array &$call_args, SearchApiQueryInterface $query) {

    $index = $query->getIndex();
    $fields = $this->getFieldNames($index);
    $field_handler = $this->getAlgorithm($query)->getFieldHandler();

    $fq = &$call_args['params']['fq'];
    // If search api location isn't present add the bbox query by ourselves.
    if (($bbox_query = $this->handleBboxFilter($query))) {
      $fq[] = $bbox_query;
    }

    // Solr params.
    $params = array(
      'group'           => 'true',
      'group.limit'     => 1,
    );
    $group_field_name = $this->getGroupingField($query);
    // Register group field if possible.
    if (isset($fields[$group_field_name])) {
      $params['group.field'] = array(SearchApiSolrConnection::escapeFieldName($fields[$group_field_name]));
    }
    // If possible add statistic params.
    $statistics_field_name = $field_handler->real_field;
    if (!isset($fields[$statistics_field_name . ':lat']) || !isset($fields[$statistics_field_name . ':lon'])) {
      // Get the base name of the field and see if the latlon property is
      // available as well.
      $statistics_field_name = $this->getLocationFieldBaseName($query);
    }
    if (isset($fields[$statistics_field_name . ':lat']) && isset($fields[$statistics_field_name . ':lon']) && isset($fields[$group_field_name])) {
      // Use Solr Stats component to calculate mean of grouped lat/lon values.
      $params['stats'] = 'true';
      $params['stats.facet'] = array(SearchApiSolrConnection::escapeFieldName($fields[$group_field_name]));
      $params['stats.field'] = array(
        SearchApiSolrConnection::escapeFieldName($fields[$statistics_field_name . ':lat']),
        SearchApiSolrConnection::escapeFieldName($fields[$statistics_field_name . ':lon']),
      );
    }

    $call_args['params'] += $params;
  }

  /**
   * Implements SearchApiServiceInterface::search().
   *
   * Some of the data determined here are used a step further:
   * @see GeoclusterSearchApiSolrService::preQuery()
   */
  public function search(SearchApiQueryInterface $query) {
    if ($this->getAlgorithm($query)) {
      // Add grouping options.
      if (($grouping_field = $this->getGroupingField($query))) {
        // Set grouping options, so search api will handle result groups.
        $search_api_grouping = $query->getOption('search_api_grouping', array());
        $search_api_grouping['use_grouping'] = TRUE;
        $search_api_grouping['fields'][] = $grouping_field;
        $query->setOption('search_api_grouping', $search_api_grouping);
      }

      // Handle the bbox filtering.
      $this->handleBboxFilter($query);
    }
    return parent::search($query);
  }

  /**
   * Apply custom changes before the query is sent to Solr.
   *
   * Works exactly like hook_search_api_solr_query_alter().
   *
   * Initial grouping and bbox handling is done a step before:
   * @see GeoclusterSearchApiSolrService::search()
   *
   * @param array $call_args
   *   An associative array containing all four arguments to the
   *   Apache_Solr_Service::search() call ("query", "offset", "limit" and
   *   "params") as references.
   * @param SearchApiQueryInterface $query
   *   The SearchApiQueryInterface object representing the executed search
   *   query.
   */
  protected function preQuery(array &$call_args, SearchApiQueryInterface $query) {
    if ($this->getAlgorithm($query)) {
      $this->addGeoclusterParams($call_args, $query);
    }
    parent::preQuery($call_args, $query);
  }

  /**
   * Implements SearchApiAutocompleteInterface::getAutocompleteSuggestions().
   */
  public function getAutocompleteSuggestions(SearchApiQueryInterface $query, SearchApiAutocompleteSearch $search, $incomplete_key, $user_input) {
    // Get geocluster from query option, as we can't access the view object.
    // @see GeoclusterSearchApiViewsQuery::build().
    $geocluster = $query->getOption("geocluster");
    if ($geocluster != NULL) {
    }
    return parent::getAutocompleteSuggestions($query, $search, $incomplete_key, $user_input);
  }


  /**
   * Extract the clustering information and add them to the results.
   *
   * @param array $results
   *   The results array that will be returned for the search.
   * @param SearchApiQueryInterface $query
   *   The SearchApiQueryInterface object representing the executed search
   *   query.
   * @param Apache_Solr_Response $response
   *   The response object returned by Solr.
   */
  protected function postQuery(array &$results, SearchApiQueryInterface $query, $response) {
    if (module_exists('devel')) {
      timer_start('geocluster');
      dd(timer_read("geocluster") . "ms: started clustering");
    }

    // Post-process clustered results.
    if (($algorithm = $this->getAlgorithm($query))) {
      geophp_load();
      $total_items = 0;
      $results_by_geohash = array();
      $index = $query->getIndex();
      $fields = $this->getFieldNames($index);
      $grouped_field = $fields[$this->getGroupingField($query)];
      $groups = $response->grouped->{$grouped_field}->groups;
      if (module_exists('devel')) {
        dd(timer_read("geocluster") . "ms: items from database: " . count($groups));
      }

      // Prepare data for getting cluster centers from solr stats component.
      // If possible add statistic params.
      $stats_field_name = $algorithm->getFieldHandler()->real_field;
      if (!isset($fields[$stats_field_name . ':lat']) || !isset($fields[$stats_field_name . ':lon'])) {
        // Get the base name of the field and see if the latlon property is
        // available as well.
        $stats_field_name = $this->getLocationFieldBaseName($query);
      }

      $stats_field_lat = $fields[$stats_field_name . ':lat'];
      $stats_field_lon = $fields[$stats_field_name . ':lon'];
      $stats = $response->stats->stats_fields;
      $stats_lat = ((isset($stats->{$stats_field_lat}->facets->{$grouped_field})) ? $stats->{$stats_field_lat}->facets->{$grouped_field} : NULL);
      $stats_lon = ((isset($stats->{$stats_field_lon}->facets->{$grouped_field})) ? $stats->{$stats_field_lon}->facets->{$grouped_field} : NULL);

      foreach ($groups as $key => &$group) {

        // Clustered results.
        // @todo: create cluster entity instead of fake result.
        $doc = $group->doclist->docs[0];
        $item_id_field = 'search_api_id';
        // Backwards compatibility.
        if (isset($fields['search_api_item_id'])) {
          $item_id_field = 'search_api_item_id';
        }
        $id = $doc->{$fields[$item_id_field]};
        $result = &$results['results'][$id];

        // Set geocluster specific result data.
        $result['clustered'] = $group->doclist->numFound > 1;
        $result['geocluster_ids'] = $id;
        $result['geocluster_count'] = $group->doclist->numFound;
        $total_items += $group->doclist->numFound;
        if ($geohash = $group->groupValue) {
          // TODO: improve accuracy?
          // $location = geoPHP::load($geohash, 'geohash');
          $location = new Point(
            $stats_lon->{$geohash}->mean,
            $stats_lat->{$geohash}->mean
          );
          $result['geocluster_geometry'] = $location;
          $result['geocluster_lat'] = $location->getY();
          $result['geocluster_lon'] = $location->getX();
        }

        $results_by_geohash[$geohash] = array(
          $id => &$result,
        );
      }

      if (module_exists('devel')) {
        dd(timer_read("geocluster") . "ms: pre-clustered by geohash: " . count($groups));
      }

      $algorithm->setValues($results['results']);
      $algorithm->clusterByNeighborCheck($results_by_geohash);

      if (module_exists('devel')) {
        dd(timer_read("geocluster") . "ms: merged & finalized clusters: " . count($results['results']));
        dd(timer_read("geocluster") . "ms: total items within clusters: " . $total_items);
        timer_stop("geocluster");
      }

    }
    else {
      parent::postQuery($results, $query, $response);
    }
  }
}
