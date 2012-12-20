<?php


class GeoclusterSearchApiEntityDataSourceController extends SearchApiEntityDataSourceController {

  var $real_type = 'node';
  var $entity_type;

  /**
   * Constructor for a data source controller.
   *
   * @param $type
   *   The item type for which this controller is created.
   */
  public function __construct($type) {
    parent::__construct($type);
    $this->entity_type = $type;
    // $this->type = $this->real_type;
  }

  /**
   * Load items of the type of this data source controller.
   *
   * @param array $ids
   *   The IDs of the items to laod.
   *
   * @return array
   *   The loaded items, keyed by ID.
   */
  public function loadItems(array $ids) {
    return parent::loadItems($ids);
    return array();
  }

  protected function checkIndex(SearchApiIndex $index) {
  }

}
