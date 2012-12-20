<?php

/**
 * @file
 * Geohash-specifics for geocluster.
 */

function geocluster_admin($callback_arg = '') {
  $build = array();
  return $build;
}

/**
 * UI controller.
 */
class GeoclusterItemTypeUIController extends EntityDefaultUIController {

  /**
   * Overrides hook_menu() defaults.
   */
  public function hook_menu() {
    $items = parent::hook_menu();
    $items[$this->path]['description'] = 'Manage geocluster';
    return $items;
  }
}



/**
 * Generates the geocluster type editing form.
 */
function geocluster_item_type_form($form, &$form_state, $geocluster_item_type, $op = 'edit') {

  if ($op == 'clone') {
    $geocluster_item_type->label .= ' (cloned)';
    $geocluster_item_type->type = '';
  }

  $form['label'] = array(
    '#title' => t('Label'),
    '#type' => 'textfield',
    '#default_value' => $geocluster_item_type->label,
    '#description' => t('The human-readable name of this geocluster item type.'),
    '#required' => TRUE,
    '#size' => 30,
  );
  // Machine-readable type name.
  $form['type'] = array(
    '#type' => 'machine_name',
    '#default_value' => isset($geocluster_item_type->type) ? $geocluster_item_type->type : '',
    '#maxlength' => 32,
    '#disabled' => $geocluster_item_type->isLocked() && $op != 'clone',
    '#machine_name' => array(
      // 'exists' => 'profile2_get_types',
      'source' => array('label'),
    ),
    '#description' => t('A unique machine-readable name for this geocluster item type. It must only contain lowercase letters, numbers, and underscores.'),
  );

  if (!$geocluster_item_type->isLocked() && $op != 'add' && $op != 'clone') {
    $form['actions']['delete'] = array(
      '#type' => 'submit',
      '#value' => t('Delete geocluster item type'),
      '#weight' => 45,
      '#limit_validation_errors' => array(),
      '#submit' => array('geocluster_item_type_form_submit_delete')
    );
  }
  return $form;
}

/**
 * Form API submit callback for the type form.
 */
function geocluster_item_type_form_submit(&$form, &$form_state) {
  $geocluster_item_type = entity_ui_form_submit_build_entity($form, $form_state);
  // Save and go back.
  $geocluster_item_type->save();
  $form_state['redirect'] = 'admin/structure/geocluster';
}

/**
 * Form API submit callback for the delete button.
 */
function geocluster_item_type_form_submit_delete(&$form, &$form_state) {
  $form_state['redirect'] = 'admin/structure/geocluster/manage/' . $form_state['geocluster_item_type']->type . '/delete';
}
