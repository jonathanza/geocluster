<?php

/**
 * @file
 * Abstract definition of a Geocluster algorithm.
 */

abstract class GeoclusterAlgorithm {

  /**
   * @var GeoclusterConfigBackendInterface
   */
  var $config;

  /**
   * Reference to Geofield field handler on which to perform clustering.
   *
   * @var views_handler_field
   */
  var $field_handler;

  /**
   * Minimum cluster distance, as defined by GeoclusterConfigViewsDisplayExtender.
   * @todo: document unit.
   *
   * @var float
   */
  var $cluster_distance;

  /**
   * Current zoom level for clustering.
   *
   * @var int
   */
  var $zoom_level;

  /**
   * Resolution in meters / pixel based on zoom_level.
   * @see GeoclusterHelper::resultions().
   *
   * @var float
   */
  var $resolution;

  /**
   * Geohash length for clustering by a specified distance in pixels.
   *
   * @var int
   */
  var $geohash_length;

  /**
   * Items being clustered.
   */
  var $values;

  /**
   * Constructor.
   *
   * @param $config
   * @param $cluster_distance
   * @param $zoom
   * @param $field_handler views_handler_field
   */
  function __construct($config, $cluster_distance, $zoom, $field_handler) {
    $this->config = $config;
    $this->field_handler = $field_handler;
    $this->cluster_distance = $cluster_distance;
    $this->zoom_level = $zoom;
    $resolutions = GeoclusterHelper::resolutions();
    $this->resolution = $resolutions[$zoom];
    $this->geohash_length = GeohashHelper::lengthFromDistance($this->cluster_distance, $this->resolution);
    $this->after_construct();
  }

  /**
   * Perform any clustering tasks before the views query will be executed.
   */
  abstract function pre_execute();

  /**
   * Perform any clustering tasks after the views query has been executed.
   */
  abstract function post_execute();


  /*** DEBUGGING-RELATED WRAPPER FUNCTIONS ***/

  function after_construct() {
    if (module_exists("devel")) {
      $debug =
        "zoom: " . $this->zoom_level .
          " , resolution: " . $this->resolution .
          " , distance: " . $this->cluster_distance .
          " , geohash_length: " . $this->geohash_length;
      dd("");
      dd($debug);
    }
  }

  function before_pre_execute() {
    if (module_exists("devel")) {
      timer_start("geocluster");
      dd("before pre execute");
    }
  }

  function before_post_execute() {
    $view = $this->config->get_view();
    $this->values = &$view->result;

    if (module_exists("devel")) {
      $query =  Database::getConnection()->prefixTables(vsprintf($view->build_info['query'], $view->build_info['query_args']));
      $replacements = module_invoke_all('views_query_substitutions', $view);
      $query = str_replace(array_keys($replacements), $replacements, $query);
      dd($query);
      dd("started clustering " . count($this->values) . " items");
    }
  }

  function after_post_execute() {
    if (module_exists("devel")) {
      dd("merged & finalized clusters: " . timer_read("geocluster"));
      timer_stop("geocluster");
    }
  }

  /*** GETTERS & SETTERS ***/

  /**
   * @return float
   */
  public function getClusterDistance() {
    return $this->cluster_distance;
  }

  /**
   * @return \views_handler_field
   */
  public function getFieldHandler() {
    return $this->field_handler;
  }

  /**
   * @return float
   */
  public function getResolution() {
    return $this->resolution;
  }

  /**
   * @return int
   */
  public function getZoomLevel() {
    return $this->zoom_level;
  }

  /**
   * @return int
   */
  public function getGeohashLength() {
    return $this->geohash_length;
  }

}
