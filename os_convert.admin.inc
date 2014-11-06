<?php
/**
 * @file
 *
 * Administration related items.
 */

define('OS_DEFAULT_MAP', '150012|150012-leksikon
150015|870970-basis
150016|870971-forfweb
150017|870971-faktalink
150018|150018-danhist
150021|150021-bibliotek
150023|150023-sicref
150025|150008-public
150027|150021-fjern
150028|870970-basis
150030|870970-spilmedier
150032|150018-samfund
150033|150033-dandyr
150034|150018-religion
150039|150015-forlag
150040|150033-verdyr
150043|150043-atlas
150048|870970-basis
150052|870970-basis
150054|150018-biologi
150055|150018-fysikkemi
150056|150018-geografi
159002|159002-lokalbibl
100200|100200-katalog
710100|710100-katalog
714700|714700-katalog
715300|715300-katalog
715700|715700-katalog
716100|716100-katalog
716300|716300-katalog
716500|716500-katalog
716700|716700-katalog
717300|717300-katalog
717500|717500-katalog
718300|718300-katalog
718500|718500-katalog
720100|720100-katalog
721700|721700-katalog
726000|726000-katalog
730600|730600-katalog
732000|732000-katalog
732900|732900-katalog
733000|733000-katalog
736000|736000-katalog
742000|742000-katalog
743000|743000-katalog
744000|744000-katalog
745000|745000-katalog
746100|746100-katalog
747900|747900-katalog
748000|748000-katalog
748200|748200-katalog
753000|753000-katalog
754000|754000-katalog
756100|756100-katalog
757300|757300-katalog
757500|757500-katalog
758000|758000-katalog
760700|760700-katalog
762100|762100-katalog
763000|763000-katalog
766500|766500-katalog
773000|773000-katalog
774000|774000-katalog
774600|774600-katalog
775100|775100-katalog
776600|776600-katalog
785100|785100-katalog
870973|870973-anmeld
870976|870976-anmeld
830380|830380-katalog
830190|830190-katalog
830690|830690-katalog
830370|830370-katalog
830060|830060-katalog
831020|831020-katalog');

/**
 * Admin form, tiny interface for conversion progress.
 */
function os_convert_admin_form($form, &$form_state) {
  $count = os_convert_objects_count(TRUE);
  $processed = os_convert_processed_count(TRUE);
  $t = array(
    '@processed' => $processed,
    '@count' => $count,
  );

  if (!isset($form_state['os_convert_init'])) {
    $form['os_convert_map'] = array(
      '#type' => 'textarea',
      '#cols' => 20,
      '#rows' => 20,
      '#default_value' => variable_get('os_convert_map', OS_DEFAULT_MAP),
      '#description' => t('Use in form of "key|value" (no quotes), where key is old agency id (OS 2.x) and value is the new agency id (OS 3.x)'),
    );

    $form['os_convert_direct'] = array(
      '#title' => t('Direct mapping'),
      '#type' => 'checkboxes',
      '#options' => os_convert_direct_mapping_files(),
      '#description' => t('Use direct OS 2.x to OS 3.x ids mapping from these files')
    );

    $form['os_convert_actions'] = array(
      '#type' => 'container',
      '#prefix' => '<div id="os-convert-actions">',
      '#suffix' => '</div>',
    );

    $processed = os_convert_processed_count(TRUE);
    $t['@processed'] = $processed;

    $form['os_convert_actions']['os_convert_progress'] = array(
      '#type' => 'item',
      '#markup' => t('Overall status (processed/total): @processed/@count', $t),
    );

    $pending = os_convert_leftover_count(TRUE);
    $t['@pending'] = $pending;
    $form['os_convert_actions']['os_convert_pending'] = array(
      '#type' => 'item',
      '#markup' => t('Not mapped: @pending', $t),
    );

    $remapped = variable_get('os_convert_remapped', FALSE);
    $form['os_convert_actions']['os_convert_remap'] = array(
      '#type' => 'submit',
      '#value' => t('Remap identifiers'),
      '#submit' => array('os_convert_admin_form_remap'),
      '#ajax' => array(
        'callback' => '_os_convert_admin_form_remap_ajax',
        'effect' => 'fade',
        'method' => 'replace',
        'wrapper' => 'os-convert-actions',
      ),
    );

    $form['os_convert_actions']['os_convert_run'] = array(
      '#type' => 'submit',
      '#value' => t('OpenSearch fetch'),
      '#disabled' => ($t['@processed'] == $t['@count'] || !$remapped) ? TRUE : FALSE,
    );

    /*
    $form['os_convert_actions']['os_convert_reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
      '#submit' => array('os_convert_admin_form_reset'),
    );
    */

    $form['os_convert_warning'] = array(
      '#type' => 'item',
      '#markup' => t('<em>Better backup your database first! Also make sure the php\'s max_execution and memory_limit values are set to adequate values (example: 300/512M)</em>'),
    );
  }
  else {
    $percentage = $t['@processed'] * 100 / $t['@count'];
    $t['@percentage'] = sprintf("%.4f", $percentage);
    $form['os_convert_progress_bar'] = array(
      '#type' => 'item',
      '#markup' => '<span class="progress">' . t('Overall progress: @processed/@count (@percentage%)', $t) . '</span><span class="loader"></span>',
      '#prefix' => '<div id="os-convert-progress">',
      '#suffix' => '</div>',
    );

    $form['os_convert_back'] = array(
      '#type' => 'item',
      '#markup' => l(t('Back (pause)'), 'admin/config/ting/os_convert'),
    );

    $form['#attached']['js'][] = drupal_get_path('module', 'os_convert') . '/js/os_convert_init.js';
  }

  $form['#attached']['css'][] = drupal_get_path('module', 'os_convert') . '/css/os_convert.css';

  return $form;
}

/**
 * Custom submit handler for admin form.
 *
 * @see os_convert_admin_form()
 */
function os_convert_admin_form_submit(&$form, &$form_state) {
  $form_state['os_convert_init'] = TRUE;
  $form_state['rebuild'] = TRUE;
}

/**
 * Custom submit handler for admin form.
 *
 * Reset the current counter state. This will make
 * next conversion start from scratch.
 *
 * @see os_convert_admin_form()
 */
/**
function os_convert_admin_form_reset(&$form, &$form_state) {
  drupal_set_message(t('Counters resetted.'));
  variable_set('os_convert_progress', 0);
  variable_set('os_convert_remapped', FALSE);

  db_query('TRUNCATE `ting_object_convert`');
  db_query('TRUNCATE `ting_object_leftover`');
}
 */

/**
 * Custom submit handler for admin form.
 *
 * This takes the mapping from the textarea an updates the id's
 * which could be mapped.
 *
 * @see os_convert_admin_form()
 */
function os_convert_admin_form_remap(&$form, &$form_state) {
  $time = microtime(TRUE);
  os_convert_update_left_over();
  $left_over = os_convert_leftover_count(TRUE);

  $input = $form_state['input'];
  $map = $input['os_convert_map'];
  $direct_map_sources = array_filter(array_values($input['os_convert_direct']));
  $array_map = os_convert_process_map($map);

  // Pre-load all files with direct mapping.
  foreach ($direct_map_sources as $source) {
    os_convert_load_direct_map($source);
  }

  // Multiple update hack. Use mysql CASE structure which is way faster
  // with thousands of records to update than any other method.
  $update_query = "UPDATE {ting_object} set `ding_entity_id` = CASE ";
  // Plain queries are much more reliable and bullet-proof.
  $cleanup_query = "DELETE FROM {ting_object_leftover} WHERE `tid` IN ";
  $start = 0;
  $all_tids = array();
  while (TRUE) {
    $select_query = db_select('ting_object_leftover', 'tiol');
    $select_query
      ->fields('tiol', array('tid', 'ding_entity_id'))
      ->range($start, OS_CONVERT_STEP);
    $result = $select_query
      ->execute()
      ->fetchAll();

    // No results at all, so no further processing is required.
    if ($start > $left_over) {
      watchdog('os_convert', "Emergency exit ($start).<br/>" . print_r($result, TRUE));
      break;
    }

    $tracker_query = db_insert('ting_object_convert')
      ->fields(array(
        'tid',
        'old_id',
        'new_id',
    ));

    $query_args = array();
    $tids = array();
    $query_cases = array();
    $run_query = FALSE;

    foreach ($result as $row) {
      $new_id = NULL;
      $tid = $row->tid;
      $ding_entity_id = urldecode($row->ding_entity_id);
      list($agency, $local_id) = explode(':', $ding_entity_id);

      // Remap the agency
      if (isset($array_map[$agency])) {
        $new_agency = $array_map[$agency];
        $new_id = $new_agency . ':' . $local_id;
      }
      // Or get a direct id to id relation, if any.
      else {
        $new_id = os_convert_seek_id_replacement($row->ding_entity_id);
      }

      if ($new_id) {
        $run_query = TRUE;
        // Used in UPDATE CASE ... WHERE clause.
        $tids[] = $tid;
        // Used in DELETE WHERE clause.
        $all_tids[] = $tid;
        // Used in UPDATE CASE ... WHERE query placeholders.
        $query_args[] = $tid;
        $query_args[] = $new_id;
        $query_cases[] = " WHEN `tid` = ? THEN ?";
        // Used in logging the converted id's.
        $tracker_query
          ->values(array(
            'tid' => $tid,
            'old_id' => $ding_entity_id,
            'new_id' => $new_id,
        ));
      }
    }

    $tids_string = implode(',', $tids);

    $query_cases[] = " ELSE `ding_entity_id`";
    $query_cases[] = " END";
    $query_cases[] = " WHERE `tid` IN (" . $tids_string . ")";

    if ($run_query) {
      $final_query = $update_query . implode('', $query_cases);

      // Update ting_object table with new id's.
      db_query($final_query, $query_args);

      // Insert rows into the ting_object_convert table
      // whose id's were converted to keep old_id => new_id relation.
      $tracker_query->execute();
    }

    $start += OS_CONVERT_STEP;
  }

  // Delete rows from ting_object_leftover table
  // who's id's were converted.
  // The reason why is this outside the main loop is to avoid
  // query LIMIT clashes. When OFFSET is higher than the actual
  // number of rows in the table.
  $i = 0;
  while ($tid_chunk = array_slice($all_tids, $i, OS_CONVERT_STEP)) {
    $tids_string = implode(',', $tid_chunk);
    db_query($cleanup_query . '(' . $tids_string . ')');
    $i += OS_CONVERT_STEP;
  }

  variable_set('os_convert_remapped', TRUE);
  os_convert_processed_count(TRUE);
  $form_state['rebuild'] = TRUE;

  $time = microtime(TRUE) - $time;
  $mem = memory_get_peak_usage(TRUE);
  $t = array(
    '@count' => os_convert_processed_count(TRUE),
    '@time' => $time,
    '@mem' => $mem,
  );
  watchdog('os_convert', 'Converted: @count; Time: @time ms.; Memory: @mem', $t);
}

/**
 * AJAX submit callback for remap button.
 *
 * Simply update the buttons.
 *
 * @see os_convert_admin_form()
 */
function _os_convert_admin_form_remap_ajax($form, &$form_state) {
  return $form['os_convert_actions'];
}

/**
 * Process the input old_id to new_id relation to
 * machine readable form.
 *
 * @param string $map
 *   Text denoting old_id to new_id format.
 *   Rows, every of which should be in form
 *   "old_id|new_id" (no quotes).
 * @return array
 *   Asociative array keyed by old id's with values of new id's.
 */
function os_convert_process_map($map) {
  // Split new lines. This should work with all line ending types.
  $map_rows = preg_split('/$\R?^/m', $map);
  $array_map = array();

  foreach ($map_rows as $row) {
    $r = trim($row);
    $r = explode('|', $r);
    $array_map[$r[0]] = $r[1];
  }

  return $array_map;
}

/**
 * Read for available files with mapping information.
 *
 * @see os_convert_admin_form()
 *
 * @return Array
 *   List of files for checkbox labels and values.
 */
function os_convert_direct_mapping_files() {
  $base = drupal_get_path('module', 'os_convert') . '/direct_map/';
  $files = scandir($base);

  foreach ($files as $k => $file) {
    if (!is_file($base . $file) || !is_readable($base . $file)) {
      unset($files[$k]);
    }
  }

  return array_combine($files, $files);
}

/**
 * Read and statically cache all data stored in mapping files.
 *
 * @param string $source
 *   Filename. Just the filename.
 * @return Object
 *   Object data structure for selected file source.
 */
function os_convert_load_direct_map($source) {
  $direct_map = &drupal_static(__FUNCTION__, array());

  if (!isset($direct_map[$source])) {
    $base = drupal_get_path('module', 'os_convert') . '/direct_map/';
    $contents = file_get_contents($base . $source);
    $direct_map[$source] = json_decode($contents);
  }

  return $direct_map[$source];
}

/**
 * Search for statically cached id mapping.
 *
 * @param string $id
 *   Item id whose replacement is searched.
 * @return mixed
 *   String if replacement is found, FALSE otherwise.
 */
function os_convert_seek_id_replacement($id) {
  $sources = &drupal_static('os_convert_load_direct_map');

  foreach ($sources as $source) {
    if (isset($source->{$id})) {
      return $source->{$id};
    }
  }

  return FALSE;
}
