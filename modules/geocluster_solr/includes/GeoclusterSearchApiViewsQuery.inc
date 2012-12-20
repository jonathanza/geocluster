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

    if ($geocluster = geocluster_get_instance($view)) {
      $this->query->setOption("geocluster", $geocluster);
    }
  }
}
