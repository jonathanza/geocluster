<?php

/**
 * Implements hook_entity_info().
 */
function geocluster_entity_info() {
  $return = array();
  $return['geocluster_item'] = array(
    'label' => t('Geocluster Item'),
    'entity class' => 'GeoclusterItem',
    'controller class' => 'GeoclusterItemController',
    'base table' => 'geocluster_item',
    'fieldable' => TRUE,
    'entity keys' => array(
      'id' => 'id',
      'bundle' => 'type',
    ),
    'bundles' => array(),
    'bundle keys' => array(
      'bundle' => 'type',
    ),
    'view modes' => array(
      'single' => array(
        'label' => t('Single item'),
        'custom settings' => TRUE,
      ),
      'cluster' => array(
        'label' => t('Cluster'),
        'custom settings' => TRUE,
      ),
    ),
    'module' => 'geocluster',
    'access callback' => 'geocluster_item_access',
  );
  $return['geocluster_item_type'] = array(
    'label' => t('Geocluster Item Type'),
    'entity class' => 'GeoclusterItemType',
    'controller class' => 'EntityAPIControllerExportable',
    'base table' => 'geocluster_item_type',
    'fieldable' => FALSE,
    'bundle of' => 'geocluster_item',
    'exportable' => TRUE,
    'entity keys' => array(
      'id' => 'id',
      'name' => 'type',
      'label' => 'label',
    ),
    'module' => 'geocluster',
    // Enable the entity API's admin UI.
    'admin ui' => array(
      'path' => 'admin/structure/geocluster',
      'file' => 'geocluster.admin.inc',
      'controller class' => 'GeoclusterItemTypeUIController',
    ),
    'access callback' => 'geocluster_item_type_access',
  );
  return $return;
}

/**
 * Implements hook_entity_info_alter().
 */
function geocluster_entity_info_alter(&$entity_info) {
  // Add geocluster item type for every entity bundle that has a geofield attached to it.
  $field_info = field_info_field_by_ids();
  foreach (field_info_instances() as $entity_type => $entity_bundles) {
    if ($entity_type != 'geocluster_item') {
      foreach ($entity_bundles as $bundle => $bundle_fields) {
        foreach ($bundle_fields as $field_info_instance) {
          $is_valid_field_instance = isset($field_info_instance['field_id']) && isset($field_info[$field_info_instance['field_id']]['type']);
          if ($is_valid_field_instance && 'geofield' === $field_info[$field_info_instance['field_id']]['type']) {
            $type = $entity_type . '__' . $bundle;
            $label = $entity_info[$entity_type]['label'] . ' / ' . $entity_info[$entity_type]['bundles'][$bundle]['label'] . ' Geocluster Item';
            $entity_info['geocluster_item']['bundles'][$type] = array(
              'label' => $label,
              'admin' => array(
                'path' => 'admin/structure/geocluster/manage/%geocluster_item_type',
                'real path' => 'admin/structure/geocluster/manage/' . $type,
                'bundle argument' => 4,
                'access arguments' => array('administer geocluster item types'),
              ),
            );
            dpm($entity_info['geocluster_item']['bundles'][$type]);
          }
        }
      }
    }
  }
}

/**
 * GeoclusterItem class.
 */
class GeoclusterItem extends Entity {

  protected function defaultLabel() {
    return $this->title;
  }
}

class GeoclusterItemController extends EntityAPIController {

  public function create(array $values = array()) {
    return parent::create($values);
  }

  public function buildContent($entity, $view_mode = 'full', $langcode = NULL, $content = array()) {
    $real_entity_type = 'node';
    $wrapper = entity_metadata_wrapper($real_entity_type, $entity);

    return parent::buildContent($entity, $view_mode, $langcode, $content);
  }

  public function save($entity, DatabaseTransaction $transaction = NULL) {
  }

  public function load($ids = array(), $conditions = array()) {
    return parent::load($ids, $conditions);
  }

  public function delete($ids, DatabaseTransaction $transaction = NULL) {
  }

}

/**
 * Geocluster Item Type class.
 */
class GeoclusterItemType extends Entity {
  public $type;
  public $label;
  public $weight = 0;
  public function __construct($values = array()) {
    parent::__construct($values, 'geocluster_item_type');
  }

  function isLocked() {
    return isset($this->status) && empty($this->is_new) && (($this->status & ENTITY_IN_CODE) || ($this->status & ENTITY_FIXED));
  }
}

class GeoclusterItemTypeController extends EntityAPIControllerExportable {
  public function create(array $values = array()) {
    $values += array(
      'label' => '',
      'description' => '',
    );
    return parent::create($values);
  }

  /**
   * Save Geocluster Item Type.
   */
  public function save($entity, DatabaseTransaction $transaction = NULL) {
    parent::save($entity, $transaction);
    // Rebuild menu registry. We do not call menu_rebuild directly, but set
    // variable that indicates rebuild in the end.
    // @see _http://drupal.org/node/1399618
    variable_set('menu_rebuild_needed', TRUE);
  }
}

/**
 * Access callback for the entity API.
 */
function geocluster_item_access($op, $type = NULL, $account = NULL) {
  return user_access('administer geocluster items', $account);
}

/**
 * Access callback for the entity API.
 */
function geocluster_item_type_access($op, $type = NULL, $account = NULL) {
  return user_access('administer geocluster item types', $account);
}
