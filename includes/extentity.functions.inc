<?php

/**
 * Determines whether for the given entity type a given operation is available.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $op
 *   One of 'create', 'view', 'save', 'delete', 'revision delete', 'access' or
 *   'form'.
 *
 * @return boolean
 *   Whether the entity type supports the given operation.
 */
function entity_type_supports($entity_type, $op) {
  $info = entity_get_info($entity_type);
  $keys = array(
    'view' => 'view callback',
    'create' => 'creation callback',
    'delete' => 'deletion callback',
    'revision delete' => 'revision deletion callback',
    'save' => 'save callback',
    'access' => 'access callback',
    'form' => 'form callback'
  );
  if (isset($info[$keys[$op]])) {
    return TRUE;
  }
  if ($op == 'revision delete') {
    return in_array('EntityAPIControllerInterface', class_implements($info['controller class']));
  }
  if ($op == 'form') {
    return (bool) entity_ui_controller($entity_type);
  }
  if ($op != 'access') {
    return in_array('EntityAPIControllerInterface', class_implements($info['controller class']));
  }
  return FALSE;
}

/**
 * Menu loader function: load an entity from its path.
 *
 * This can be used to load entities of all types in menu paths:
 *
 * @code
 * $items['myentity/%entity_object'] = array(
 *   'load arguments' => array('myentity'),
 *   'title' => ...,
 *   'page callback' => ...,
 *   'page arguments' => array(...),
 *   'access arguments' => array(...),
 * );
 * @endcode
 *
 * @param $entity_id
 *   The ID of the entity to load, passed by the menu URL.
 * @param $entity_type
 *   The type of the entity to load.
 * @return
 *   A fully loaded entity object, or FALSE in case of error.
 */
function entity_object_load($entity_id, $entity_type) {
  $entities = entity_load($entity_type, array($entity_id));
  return reset($entities);
}

/**
 * Page callback to show links to add an entity of a specific bundle.
 *
 * Entity modules that provide a further description to their bundles may wish
 * to implement their own version of this to show these.
 *
 * @param $entity_type
 *   The type of the entity.
 */
function entity_ui_bundle_add_page($entity_type) {
  // Set the title, as we're a MENU_LOCAL_ACTION and hence just get tab titles.
  module_load_include('inc', 'rules', 'includes/extentity.ui');
  backdrop_set_title(entity_ui_get_action_title('add', $entity_type));

  // Get entity info for our bundles.
  $info = entity_get_info($entity_type);
  $items = array();
  foreach ($info['bundles'] as $bundle_name => $bundle_info) {
    // Create an empty entity with just the bundle set to check for access.
    $dummy_entity = entity_create($entity_type, array(
      $info['entity keys']['bundle'] => $bundle_name,
    ));
    // If modules use a uid, they can default to the current-user
    // in their create() method on the storage controller.
    if (entity_access('create', $entity_type, $dummy_entity, $account = NULL)) {
      $add_path = $info['admin ui']['path'] . '/add/' . $bundle_name;
      $items[] = l(t('Add @label', array('@label' => $bundle_info['label'])), $add_path);
    }
  }
  return theme('item_list', array('items' => $items));
}

/**
 * Page callback to add an entity of a specific bundle.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $bundle_name
 *   The bundle machine name.
 */
function entity_ui_get_bundle_add_form($entity_type, $bundle_name) {
  $info = entity_get_info($entity_type);
  $bundle_key = $info['entity keys']['bundle'];

  // Make a stub entity of the right bundle to pass to the entity_ui_get_form().
  $values = array(
    $bundle_key => $bundle_name,
  );
  $entity = entity_create($entity_type, $values);

  return entity_ui_get_form($entity_type, $entity, 'add');
}

/**
 * Page callback for viewing an entity.
 *
 * @param Entity $entity
 *   The entity to be rendered.
 *
 * @return array
 *   A renderable array of the entity in full view mode.
 */
function entity_ui_entity_page_view($entity) {
  module_load_include('inc', 'rules', 'includes/extentity.ui');
  return $entity->view('full', NULL, TRUE);
}

/**
 * Gets the page title for the passed operation.
 */
function entity_ui_get_page_title($op, $entity_type, $entity = NULL) {
  if (isset($entity)) {
    module_load_include('inc', 'rules', 'includes/extentity.ui');
    $label = entity_label($entity_type, $entity);
    list(, , $bundle) = entity_extract_ids($entity_type, $entity);
  }
  else {
    $info = entity_get_info($entity_type);
    $label = $info['label'];
    $bundle = NULL;
  }

  switch ($op) {
    case 'view':
      return $label;
    case 'edit':
      return t('Edit @label', array('@label' => $label));
    case 'clone':
      return t('Clone @label', array('@label' => $label));
    case 'revert':
      return t('Revert @label', array('@label' => $label));
    case 'delete':
      return t('Delete @label', array('@label' => $label));
    case 'export':
      return t('Export @label', array('@label' => $label));
  }

  return entity_ui_get_action_title($op, $entity_type, $bundle);
}

/**
 * A wrapper around entity_load() to load a single entity by name or numeric id.
 *
 * @todo: Re-name entity_load() to entity_load_multiple() in d8 core and this
 * to entity_load().
 *
 * @param $entity_type
 *   The entity type to load, e.g. node or user.
 * @param $id
 *   The entity id, either the numeric id or the entity name. In case the entity
 *   type has specified a name key, both the numeric id and the name may be
 *   passed.
 *
 * @return
 *   The entity object, or FALSE.
 *
 * @see entity_load()
 */
function entity_load_single($entity_type, $id) {
  $entities = entity_load($entity_type, array($id));
  return reset($entities);
}

/**
 * A wrapper around entity_load() to return entities keyed by name key if existing.
 *
 * @param $entity_type
 *   The entity type to load, e.g. node or user.
 * @param $names
 *   An array of entity names or ids, or FALSE to load all entities.
 * @param $conditions
 *   (deprecated) An associative array of conditions on the base table, where
 *   the keys are the database fields and the values are the values those
 *   fields must have. Instead, it is preferable to use EntityFieldQuery to
 *   retrieve a list of entity IDs loadable by this function.
 *
 * @return
 *   An array of entity objects indexed by their names (or ids if the entity
 *   type has no name key).
 *
 * @see entity_load()
 */
function entity_load_multiple_by_name($entity_type, $names = FALSE, $conditions = array()) {
  $entities = entity_load($entity_type, $names, $conditions);
  $info = entity_get_info($entity_type);
  if (!isset($info['entity keys']['name'])) {
    return $entities;
  }
  return entity_key_array_by_property($entities, $info['entity keys']['name']);
}

/**
 * Permanently save an entity.
 *
 * In case of failures, an exception is thrown.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entity
 *   The entity to save.
 *
 * @return
 *   For entity types provided by the CRUD API, SAVED_NEW or SAVED_UPDATED is
 *   returned depending on the operation performed. If there is no information
 *   how to save the entity, FALSE is returned.
 *
 * @see entity_type_supports()
 */
function entity_save($entity_type, $entity) {
  $info = entity_get_info($entity_type);
  if (method_exists($entity, 'save')) {
    return $entity->save();
  }
  elseif (isset($info['save callback'])) {
    $info['save callback']($entity);
  }
  elseif (in_array('EntityAPIControllerInterface', class_implements($info['controller class']))) {
  print('efinc245');
    return entity_get_controller($entity_type)->save($entity);
  }
  else {
    return FALSE;
  }
}

/**
 * Permanently delete the given entity.
 *
 * In case of failures, an exception is thrown.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $id
 *   The uniform identifier of the entity to delete.
 *
 * @return
 *   FALSE, if there were no information how to delete the entity.
 *
 * @see entity_type_supports()
 */
function entity_delete($entity_type, $id) {
  return entity_delete_multiple($entity_type, array($id));
}

/**
 * Loads an entity revision.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $revision_id
 *   The id of the revision to load.
 *
 * @return
 *   The entity object, or FALSE if there is no entity with the given revision
 *   id.
 */
function entity_revision_load($entity_type, $revision_id) {
  $info = entity_get_info($entity_type);
  if (!empty($info['entity keys']['revision'])) {
    $entity_revisions = entity_load($entity_type, FALSE, array($info['entity keys']['revision'] => $revision_id));
    return reset($entity_revisions);
  }
  return FALSE;
}

/**
 * Deletes an entity revision.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $revision_id
 *   The revision ID to delete.
 *
 * @return
 *   TRUE if the entity revision could be deleted, FALSE otherwise.
 */
function entity_revision_delete($entity_type, $revision_id) {
  $info = entity_get_info($entity_type);
  if (isset($info['revision deletion callback'])) {
    return $info['revision deletion callback']($revision_id, $entity_type);
  }
  elseif (in_array('EntityAPIControllerRevisionableInterface', class_implements($info['controller class']))) {
  print('efi310');
    return entity_get_controller($entity_type)->deleteRevision($revision_id);
  }
  return FALSE;
}

/**
 * Checks whether the given entity is the default revision.
 *
 * Note that newly created entities will always be created in default revision,
 * thus TRUE is returned for not yet saved entities.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entity
 *   The entity object to check.
 *
 * @return boolean
 *   A boolean indicating whether the entity is in default revision is returned.
 *   If the entity is not revisionable or is new, TRUE is returned.
 *
 * @see entity_revision_set_default()
 */
function entity_revision_is_default($entity_type, $entity) {
  $info = entity_get_info($entity_type);
  if (empty($info['entity keys']['revision'])) {
    return TRUE;
  }
  // Newly created entities will always be created in default revision.
  if (!empty($entity->is_new) || empty($entity->{$info['entity keys']['id']})) {
    return TRUE;
  }
  if (in_array('EntityAPIControllerRevisionableInterface', class_implements($info['controller class']))) {
    $key = !empty($info['entity keys']['default revision']) ? $info['entity keys']['default revision'] : 'default_revision';
    return !empty($entity->$key);
  }
  else {
    // Else, just load the default entity and compare the ID. Usually, the
    // entity should be already statically cached anyway.
    $default = entity_load_single($entity_type, $entity->{$info['entity keys']['id']});
    return $default->{$info['entity keys']['revision']} == $entity->{$info['entity keys']['revision']};
  }
}

/**
 * Sets a given entity revision as default revision.
 *
 * Note that the default revision flag will only be supported by entity types
 * based upon the EntityAPIController, i.e. implementing the
 * EntityAPIControllerRevisionableInterface.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entity
 *   The entity revision to update.
 *
 * @see entity_revision_is_default()
 */
function entity_revision_set_default($entity_type, $entity) {
  $info = entity_get_info($entity_type);
  if (!empty($info['entity keys']['revision'])) {
    $key = !empty($info['entity keys']['default revision']) ? $info['entity keys']['default revision'] : 'default_revision';
    $entity->$key = TRUE;
  }
}

/**
 * Exports an entity.
 *
 * Note: Currently, this only works for entity types provided with the entity
 * CRUD API.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entity
 *   The entity to export.
 * @param $prefix
 *   An optional prefix for each line.
 * @return
 *   The exported entity as serialized string. The format is determined by the
 *   respective entity controller, e.g. it is JSON for the EntityAPIController.
 *   The output is suitable for entity_import().
 */
function entity_export($entity_type, $entity, $prefix = '') {
  if (method_exists($entity, 'export')) {
    return $entity->export($prefix);
  }
  $info = entity_get_info($entity_type);
  if (in_array('EntityAPIControllerInterface', class_implements($info['controller class']))) {
  print('efi399');
    return entity_get_controller($entity_type)->export($entity, $prefix);
  }
}

/**
 * Imports an entity.
 *
 * Note: Currently, this only works for entity types provided with the entity
 * CRUD API.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param string $export
 *   The string containing the serialized entity as produced by
 *   entity_export().
 * @return
 *   The imported entity object not yet saved.
 */
function entity_import($entity_type, $export) {
  $info = entity_get_info($entity_type);
  if (in_array('EntityAPIControllerInterface', class_implements($info['controller class']))) {
  print('efi421');
    return entity_get_controller($entity_type)->import($export);
  }
}

/**
 * Checks whether an entity type is fieldable.
 *
 * @param $entity_type
 *   The type of the entity.
 *
 * @return
 *   TRUE if the entity type is fieldable, FALSE otherwise.
 */
function entity_type_is_fieldable($entity_type) {
  $info = entity_get_info($entity_type);
  return !empty($info['fieldable']);
}

/**
 * Builds a structured array representing the entity's content.
 *
 * The content built for the entity will vary depending on the $view_mode
 * parameter.
 *
 * Note: Currently, this only works for entity types provided with the entity
 * CRUD API.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entity
 *   An entity object.
 * @param $view_mode
 *   A view mode as used by this entity type, e.g. 'full', 'teaser'...
 * @param $langcode
 *   (optional) A language code to use for rendering. Defaults to the global
 *   content language of the current request.
 * @return
 *   The renderable array.
 */
function entity_build_content($entity_type, $entity, $view_mode = 'full', $langcode = NULL) {
  $info = entity_get_info($entity_type);
  if (method_exists($entity, 'buildContent')) {
    return $entity->buildContent($view_mode, $langcode);
  }
  elseif (in_array('EntityAPIControllerInterface', class_implements($info['controller class']))) {
  print('efi464');
    return entity_get_controller($entity_type)->buildContent($entity, $view_mode, $langcode);
  }
}

/**
 * Returns the entity identifier, i.e. the entities name or numeric id.
 *
 * Unlike entity_extract_ids() this function returns the name of the entity
 * instead of the numeric id, in case the entity type has specified a name key.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entity
 *   An entity object.
 *
 * @see entity_extract_ids()
 */
function entity_id($entity_type, $entity) {
  if (method_exists($entity, 'identifier')) {
    return $entity->identifier();
  }
  $info = entity_get_info($entity_type);
  $key = isset($info['entity keys']['name']) ? $info['entity keys']['name'] : $info['entity keys']['id'];
  return isset($entity->$key) ? $entity->$key : NULL;
}

/**
 * Generate an array for rendering the given entities.
 *
 * Entities being viewed, are generally expected to be fully-loaded entity
 * objects, thus have their name or id key set. However, it is possible to
 * view a single entity without any id, e.g. for generating a preview during
 * creation.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entities
 *   An array of entities to render.
 * @param $view_mode
 *   A view mode as used by this entity type, e.g. 'full', 'teaser'...
 * @param $langcode
 *   (optional) A language code to use for rendering. Defaults to the global
 *   content language of the current request.
 * @param $page
 *   (optional) If set will control if the entity is rendered: if TRUE
 *   the entity will be rendered without its title, so that it can be embeded
 *   in another context. If FALSE the entity will be displayed with its title
 *   in a mode suitable for lists.
 *   If unset, the page mode will be enabled if the current path is the URI
 *   of the entity, as returned by entity_uri().
 *   This parameter is only supported for entities which controller is a
 *   EntityAPIControllerInterface.
 * @return
 *   The renderable array, keyed by the entity type and by entity identifiers,
 *   for which the entity name is used if existing - see entity_id(). If there
 *   is no information on how to view an entity, FALSE is returned.
 */
function entity_view($entity_type, $entities, $view_mode = 'full', $langcode = NULL, $page = NULL) {
  $info = entity_get_info($entity_type);
  if (isset($info['view callback'])) {
    $entities = entity_key_array_by_property($entities, $info['entity keys']['id']);
    return $info['view callback']($entities, $view_mode, $langcode, $entity_type);
  }
  elseif (in_array('EntityAPIControllerInterface', class_implements($info['controller class']))) {
  print('efi532');
    return entity_get_controller($entity_type)->view($entities, $view_mode, $langcode, $page);
  }
  return FALSE;
}

/**
 * Determines whether the given user can perform actions on an entity.
 *
 * For create operations, the pattern is to create an entity and then
 * check if the user has create access.
 *
 * @code
 * $node = entity_create('node', array('type' => 'page'));
 * $access = entity_access('create', 'node', $node, $account);
 * @endcode
 *
 * @param $op
 *   The operation being performed. One of 'view', 'update', 'create' or
 *   'delete'.
 * @param $entity_type
 *   The entity type of the entity to check for.
 * @param $entity
 *   Optionally an entity to check access for. If no entity is given, it will be
 *   determined whether access is allowed for all entities of the given type.
 * @param $account
 *   The user to check for. Leave it to NULL to check for the global user.
 *
 * @return boolean
 *   Whether access is allowed or not. If the entity type does not specify any
 *   access information, NULL is returned.
 *
 * @see entity_type_supports()
 */
function entity_access($op, $entity_type, $entity = NULL, $account = NULL) {
  if (($info = entity_get_info()) && isset($info[$entity_type]['access callback'])) {
    return $info[$entity_type]['access callback']($op, $entity, $account, $entity_type);
  }
}

/**
 * Gets the edit form for any entity.
 *
 * This helper makes use of backdrop_get_form() and the regular form builder
 * function of the entity type to retrieve and process the form as usual.
 *
 * In order to use this helper to show an entity add form, the new entity object
 * can be created via entity_create() or entity_property_values_create_entity().
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entity
 *   The entity to show the edit form for.
 * @return
 *   The renderable array of the form. If there is no entity form or missing
 *   metadata, FALSE is returned.
 *
 * @see entity_type_supports()
 */
function entity_form($entity_type, $entity) {
  $info = entity_get_info($entity_type);
  if (isset($info['form callback'])) {
    return $info['form callback']($entity, $entity_type);
  }
  // If there is an UI controller, the providing module has to implement the
  // entity form using entity_ui_get_form().
  elseif (entity_ui_controller($entity_type)) {
    return entity_metadata_form_entity_ui($entity, $entity_type);
  }
  return FALSE;
}

/**
 * Converts an array of entities to be keyed by the values of a given property.
 *
 * @param array $entities
 *   The array of entities to convert.
 * @param $property
 *   The name of entity property, by which the array should be keyed. To get
 *   reasonable results, the property has to have unique values.
 *
 * @return array
 *   The same entities in the same order, but keyed by their $property values.
 */
function entity_key_array_by_property(array $entities, $property) {
  $ret = array();
  foreach ($entities as $entity) {
    $key = isset($entity->$property) ? $entity->$property : NULL;
    $ret[$key] = $entity;
  }
  return $ret;
}

/**
 * Get the entity info for the entity types provided via the entity CRUD API.
 *
 * @return
 *  An array in the same format as entity_get_info(), containing the entities
 *  whose controller class implements the EntityAPIControllerInterface.
 */
function entity_crud_get_info() {
  $types = array();
  foreach (entity_get_info() as $type => $info) {
    if (isset($info['controller class']) && in_array('EntityAPIControllerInterface', class_implements($info['controller class']))) {
      $types[$type] = $info;
    }
  }
  return $types;
}

/**
 * Checks if a given entity has a certain exportable status.
 *
 * @param $entity_type
 *   The type of the entity.
 * @param $entity
 *   The entity to check the status on.
 * @param $status
 *   The constant status like ENTITY_CUSTOM, ENTITY_IN_CODE, ENTITY_OVERRIDDEN
 *   or ENTITY_FIXED.
 *
 * @return
 *   TRUE if the entity has the status, FALSE otherwise.
 */
function entity_has_status($entity_type, $entity, $status) {
  $info = entity_get_info($entity_type);
  $status_key = empty($info['entity keys']['status']) ? 'status' : $info['entity keys']['status'];
  return isset($entity->{$status_key}) && ($entity->{$status_key} & $status) == $status;
}

/**
 * Export a variable. Copied from ctools.
 *
 * This is a replacement for var_export(), allowing us to more nicely
 * format exports. It will recurse down into arrays and will try to
 * properly export bools when it can.
 */
function entity_var_export($var, $prefix = '') {
  if (is_array($var)) {
    if (empty($var)) {
      $output = 'array()';
    }
    else {
      $output = "array(\n";
      foreach ($var as $key => $value) {
        $output .= "  '$key' => " . entity_var_export($value, '  ') . ",\n";
      }
      $output .= ')';
    }
  }
  elseif (is_bool($var)) {
    $output = $var ? 'TRUE' : 'FALSE';
  }
  else {
    $output = var_export($var, TRUE);
  }

  if ($prefix) {
    $output = str_replace("\n", "\n$prefix", $output);
  }
  return $output;
}

/**
 * Export a variable in pretty formatted JSON.
 */
function entity_var_json_export($var, $prefix = '') {
  if (is_array($var) && $var) {
    // Defines whether we use a JSON array or object.
    $use_array = ($var == array_values($var));
    $output = $use_array ? "[" : "{";

    foreach ($var as $key => $value) {
      if ($use_array) {
        $values[] = entity_var_json_export($value, '  ');
      }
      else {
        $values[] = entity_var_json_export((string) $key, '  ') . ' : ' . entity_var_json_export($value, '  ');
      }
    }
    // Use several lines for long content. However for objects with a single
    // entry keep the key in the first line.
    if (strlen($content = implode(', ', $values)) > 70 && ($use_array || count($values) > 1)) {
      $output .= "\n  " . implode(",\n  ", $values) . "\n";
    }
    elseif (strpos($content, "\n") !== FALSE) {
      $output .= " " . $content . "\n";
    }
    else {
      $output .= " " . $content . ' ';
    }
    $output .= $use_array ? ']' : '}';
  }
  else {
    $output = backdrop_json_encode($var);
  }

  if ($prefix) {
    $output = str_replace("\n", "\n$prefix", $output);
  }
  return $output;
}

/**
 * Rebuild the default entities provided in code.
 *
 * Exportable entities provided in code get saved to the database once a module
 * providing defaults in code is activated. This allows module and entity_load()
 * to easily deal with exportable entities just by relying on the database.
 *
 * The defaults get rebuilt if the cache is cleared or new modules providing
 * defaults are enabled, such that the defaults in the database are up to date.
 * A default entity gets updated with the latest defaults in code during rebuild
 * as long as the default has not been overridden. Once a module providing
 * defaults is disabled, its default entities get removed from the database
 * unless they have been overridden. In that case the overridden entity is left
 * in the database, but its status gets updated to 'custom'.
 *
 * @param $entity_types
 *   (optional) If specified, only the defaults of the given entity types are
 *   rebuilt.
 */
function entity_defaults_rebuild($entity_types = NULL) {
  if (!isset($entity_types)) {
    $entity_types = array();
    foreach (entity_crud_get_info() as $type => $info) {
      if (!empty($info['exportable'])) {
        $entity_types[] = $type;
      }
    };
  }
  foreach ($entity_types as $type) {
    _entity_defaults_rebuild($type);
  }
}

/**
 * Actually rebuild the defaults of a given entity type.
 */
function _entity_defaults_rebuild($entity_type) {
  if (lock_acquire('entity_rebuild_' . $entity_type)) {
    $info = entity_get_info($entity_type);
    $hook = isset($info['export']['default hook']) ? $info['export']['default hook'] : 'default_' . $entity_type;
    $keys = $info['entity keys'] + array('module' => 'module', 'status' => 'status', 'name' => $info['entity keys']['id']);

    // Check for the existence of the module and status columns.
    if (!in_array($keys['status'], $info['schema_fields_sql']['base table']) || !in_array($keys['module'], $info['schema_fields_sql']['base table'])) {
      trigger_error("Missing database columns for the exportable entity $entity_type as defined by entity_exportable_schema_fields(). Update the according module and run update.php!", E_USER_WARNING);
      return;
    }

    // Invoke the hook and collect default entities.
    $entities = array();
    foreach (module_implements($hook) as $module) {
      foreach ((array) module_invoke($module, $hook) as $name => $entity) {
        $entity->{$keys['name']} = $name;
        $entity->{$keys['module']} = $module;
        $entities[$name] = $entity;
      }
    }
    backdrop_alter($hook, $entities);

    // Check for defaults that disappeared.
    $existing_defaults = entity_load_multiple_by_name($entity_type, FALSE, array($keys['status'] => array(ENTITY_OVERRIDDEN, ENTITY_IN_CODE, ENTITY_FIXED)));

    foreach ($existing_defaults as $name => $entity) {
      if (empty($entities[$name])) {
        $entity->is_rebuild = TRUE;
        if (entity_has_status($entity_type, $entity, ENTITY_OVERRIDDEN)) {
          $entity->{$keys['status']} = ENTITY_CUSTOM;
          entity_save($entity_type, $entity);
        }
        else {
          entity_delete($entity_type, $name);
        }
        unset($entity->is_rebuild);
      }
    }

    // Load all existing entities.
    $existing_entities = entity_load_multiple_by_name($entity_type, array_keys($entities));

    foreach ($existing_entities as $name => $entity) {
      if (entity_has_status($entity_type, $entity, ENTITY_CUSTOM)) {
        // If the entity already exists but is not yet marked as overridden, we
        // have to update the status.
        if (!entity_has_status($entity_type, $entity, ENTITY_OVERRIDDEN)) {
          $entity->{$keys['status']} |= ENTITY_OVERRIDDEN;
          $entity->{$keys['module']} = $entities[$name]->{$keys['module']};
          $entity->is_rebuild = TRUE;
          entity_save($entity_type, $entity);
          unset($entity->is_rebuild);
        }

        // The entity is overridden, so we do not need to save the default.
        unset($entities[$name]);
      }
    }

    // Save defaults.
    $originals = array();
    foreach ($entities as $name => $entity) {
      if (!empty($existing_entities[$name])) {
        // Make sure we are updating the existing default.
        $entity->{$keys['id']} = $existing_entities[$name]->{$keys['id']};
        unset($entity->is_new);
      }
      // Pre-populate $entity->original as we already have it. So we avoid
      // loading it again.
      $entity->original = !empty($existing_entities[$name]) ? $existing_entities[$name] : FALSE;
      // Keep original entities for hook_{entity_type}_defaults_rebuild()
      // implementations.
      $originals[$name] = $entity->original;

      if (!isset($entity->{$keys['status']})) {
        $entity->{$keys['status']} = ENTITY_IN_CODE;
      }
      else {
        $entity->{$keys['status']} |= ENTITY_IN_CODE;
      }
      $entity->is_rebuild = TRUE;
      entity_save($entity_type, $entity);
      unset($entity->is_rebuild);
    }

    // Invoke an entity type-specific hook so modules may apply changes, e.g.
    // efficiently rebuild caches.
    module_invoke_all($entity_type . '_defaults_rebuild', $entities, $originals);

    lock_release('entity_rebuild_' . $entity_type);
  }
}

/**
 * Implements hook_modules_installed().
 */
function entity_modules_installed($modules) {
  entity_entitycache_installed_modules($modules);
}



/**
 * Gets all entity types for which defaults are provided by the $modules.
 */
function _entity_modules_get_default_types($modules) {
  $types = array();
  foreach (entity_crud_get_info() as $entity_type => $info) {
    if (!empty($info['exportable'])) {
      $hook = isset($info['export']['default hook']) ? $info['export']['default hook'] : 'default_' . $entity_type;
      foreach ($modules as $module) {
        if (module_hook($module, $hook) || module_hook($module, $hook . '_alter')) {
          $types[] = $entity_type;
        }
      }
    }
  }
  return $types;
}

/**
 * Defines schema fields required for exportable entities.
 *
 * Warning: Do not call this function in your module's hook_schema()
 * implementation or update functions. It is not safe to call functions of
 * dependencies at this point. Instead of calling the function, just copy over
 * the content.
 * For more details see the issue http://drupal.org/node/1122812.
 */
function entity_exportable_schema_fields($module_col = 'module', $status_col = 'status') {
  return array(
    $status_col => array(
      'type' => 'int',
      'not null' => TRUE,
      // Set the default to ENTITY_CUSTOM without using the constant as it is
      // not safe to use it at this point.
      'default' => 0x01,
      'size' => 'tiny',
      'description' => 'The exportable status of the entity.',
    ),
    $module_col => array(
      'description' => 'The name of the providing module if the entity has been defined in code.',
      'type' => 'varchar',
      'length' => 255,
      'not null' => FALSE,
    ),
  );
}


/**
 * Determines the UI controller class for a given entity type.
 *
 * @return EntityDefaultUIController
 *   If a type is given, the controller for the given entity type. Else an array
 *   of all enabled UI controllers keyed by entity type is returned.
 */
function entity_ui_controller($type = NULL) {
  $static = &backdrop_static(__FUNCTION__);

  if (!isset($type)) {
    // Invoke the function for each type to ensure we have fully populated the
    // static variable.
    foreach (entity_get_info() as $entity_type => $info) {
      entity_ui_controller($entity_type);
    }
    return array_filter($static);
  }

  if (!isset($static[$type])) {
    $info = entity_get_info($type);
    $class = isset($info['admin ui']['controller class']) ? $info['admin ui']['controller class'] : 'EntityDefaultUIController';
    $static[$type] = (isset($info['admin ui']['path']) && $class) ? new $class($type, $info) : FALSE;
  }

  return $static[$type];
}

/**
 * Implements hook_menu().
 *
 * @see EntityDefaultUIController::hook_menu()
 */
function entity_menu() {
  $items = array();
  foreach (entity_ui_controller() as $controller) {
    $items += $controller->hook_menu();
  }
  return $items;
}




/**
 * Gets the page/menu title for local action operations.
 *
 * @param $op
 *  The current operation. One of 'add' or 'import'.
 * @param $entity_type
 *  The entity type.
 * @param $bundle_name
 *  (Optional) The name of the bundle. May be NULL if the bundle name is not
 *  relevant to the current page. If the entity type has only one bundle, or no
 *  bundles, this will be the same as the entity type.
 */
function entity_ui_get_action_title($op, $entity_type, $bundle_name = NULL) {
  $info = entity_get_info($entity_type);
  switch ($op) {
    case 'add':
      if (isset($bundle_name) && $bundle_name != $entity_type) {
        return t('Add @bundle_name @entity_type', array(
          '@bundle_name' => backdrop_strtolower($info['bundles'][$bundle_name]['label']),
          '@entity_type' => backdrop_strtolower($info['label']),
        ));
      }
      else {
        return t('Add @entity_type', array('@entity_type' => backdrop_strtolower($info['label'])));
      }
    case 'import':
      return t('Import @entity_type', array('@entity_type' => backdrop_strtolower($info['label'])));
  }
}

/**
 * Gets the extra field controller class for a given entity type.
 *
 * @return EntityExtraFieldsControllerInterface|false
 *   The controller for the given entity type or FALSE if none is specified.
 */
function entity_get_extra_fields_controller($type = NULL) {
  $static = &backdrop_static(__FUNCTION__);

  if (!isset($static[$type])) {
    $static[$type] = FALSE;
    $info = entity_get_info($type);
    if (!empty($info['extra fields controller class'])) {
      $static[$type] = new $info['extra fields controller class']($type);
    }
  }
  return $static[$type];
}

/**
 * Returns a property wrapper for the given data.
 *
 * If an entity is wrapped, the wrapper can be used to retrieve further wrappers
 * for the entitity properties. For that the wrapper support chaining, e.g. you
 * can use a node wrapper to get the node authors mail address:
 *
 * @code
 *   echo $wrappedNode->author->mail->value();
 * @endcode
 *
 * @param $type
 *   The type of the passed data.
 * @param $data
 *   The data to wrap. It may be set to NULL, so the wrapper can be used
 *   without any data for getting information about properties.
 * @param $info
 *   (optional) Specify additional information for the passed data:
 *    - langcode: (optional) If the data is language specific, its langauge
 *      code. Defaults to NULL, what means language neutral.
 *    - bundle: (optional) If an entity is wrapped but not passed, use this key
 *      to specify the bundle to return a wrapper for.
 *    - property info: (optional) May be used to use a wrapper with an arbitrary
 *      data structure (type 'struct'). Use this key for specifying info about
 *      properties in the same structure as used by hook_entity_property_info().
 *    - property info alter: (optional) A callback for altering the property
 *      info before it is utilized by the wrapper.
 *    - property defaults: (optional) An array of defaults for the info of
 *      each property of the wrapped data item.
 * @return EntityMetadataWrapper
 *   Dependend on the passed data the right wrapper is returned.
 */
function entity_metadata_wrapper($type, $data = NULL, array $info = array()) {
  if ($type == 'entity' || (($entity_info = entity_get_info()) && isset($entity_info[$type]))) {
    // If the passed entity is the global $user, we load the user object by only
    // passing on the user id. The global user is not a fully loaded entity.
    if ($type == 'user' && is_object($data) && $data == $GLOBALS['user']) {
      $data = $data->uid;
    }
    return new EntityBackdropWrapper($type, $data, $info);
  }
  elseif ($type == 'list' || entity_property_list_extract_type($type)) {
    return new EntityListWrapper($type, $data, $info);
  }
  elseif (isset($info['property info'])) {
    return new EntityStructureWrapper($type, $data, $info);
  }
  else {
    return new EntityValueWrapper($type, $data, $info);
  }
}

/**
 * Returns a metadata wrapper for accessing site-wide properties.
 *
 * Although there is no 'site' entity or such, modules may provide info about
 * site-wide properties using hook_entity_property_info(). This function returns
 * a wrapper for making use of this properties.
 *
 * @return EntityMetadataWrapper
 *   A wrapper for accessing site-wide properties.
 *
 * @see entity_metadata_system_entity_property_info()
 */
function entity_metadata_site_wrapper() {
  $site_info = entity_get_property_info('site');
  $info['property info'] = $site_info['properties'];
  return entity_metadata_wrapper('site', FALSE, $info);
}

/**
* The following functions are copied from Drupal's EntityAPI entity.install
*
*/

/**
 * Create cache tables for entities of modules that support Entity cache module.
 *
 * @param $modules
 *   (optional) An array of module names that have been installed.
 *   If not specified, try to add cache tables for all modules.
 */
function entity_entitycache_installed_modules($modules = NULL) {
  if (!module_exists('entitycache')) {
    return;
  }

  // If no modules are specified or if entitycache is being installed,
  // try to add entitycache tables for supporting entities of all modules.
  if (!isset($modules) || in_array('entitycache', $modules)) {
    $modules = module_list();
  }

  // Get all installed modules that support entity cache.
  $entitycache_module_info = _entity_entitycache_get_module_info($modules);

  // For uninstallation of modules, we need to keep a list of tables we created
  // per module providing the entity type.
  $tables_created = config_get('rules.settings','entity_cache_tables_created');

  foreach ($entitycache_module_info as $module => $module_entitycache_entities) {
    foreach ($module_entitycache_entities as $entity_type => $entity_info) {
      // Do not break modules that create the cache tables for themselves.
      if (!db_table_exists('cache_entity_' . $entity_type)) {
        $schema = backdrop_get_schema_unprocessed('system', 'cache');
        $schema['description'] = 'Cache table used to store' . $entity_type . ' entity records.';
        db_create_table('cache_entity_' . $entity_type, $schema);
        $tables_created[$module][] = 'cache_entity_' . $entity_type;
      }
    }
  }
  config_set('rules.settings','entity_cache_tables_created', $tables_created);
}

/**
 * Remove entity cache tables for entity types of uninstalled modules.
 *
 * @param $modules
 *   (optional) An array of uninstalled modules. If not specified, try to remove
 *   cache tables for all modules.
 */
function entity_entitycache_uninstalled_modules($modules = NULL) {
  // If no modules are specified or if entitycache is being uninstalled,
  // try to remove entitycache tables for supporting entities of all modules.
  if (!isset($modules) || in_array('entitycache', $modules)) {
    $modules = module_list();
  }
  $tables_created = config_get('rules.settings','entity_cache_tables_created');
  foreach ($modules as $module) {
    if (!empty($tables_created[$module])) {
      foreach ($tables_created[$module] as $table) {
        db_drop_table($table);
      }
      unset($tables_created[$module]);
    }
  }
  config_set('rules.settings','entity_cache_tables_created', $tables_created);
}

/**
 * Helper to fetch entity info about entity types that use caching.
 */
function _entity_entitycache_get_module_info($modules) {
  // Prepare a keyed array of all modules with their entity types and infos.
  // Structure: [module][entity][info]
  $entity_crud_info = entity_crud_get_info();
  $info = array();
  foreach ($entity_crud_info as $entity_name => $entity_info) {
    // Make sure that the entity info specifies a module and supports entitycache.
    if (!isset($entity_info['module']) || empty($entity_info['entity cache'])) {
      continue;
    }
    $module = $entity_info['module'];
    // Only treat installed modules.
    if (!in_array($module, $modules)) {
      continue;
    }
    // Add the entity info to the module key.
    if (!isset($info[$module])) {
      $info[$module] = array();
    }
    $info[$module][$entity_name] = $entity_info;
  }
  return $info;
}