<?php
/**
 * @file
 *
 * Administration related items.
 */

define('OS_DEFAULT_MAP', '150005|150005-artikel
150008|150008-academic
150012|150012-leksikon
150014|150014-album
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
870971|870971-avis
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
      '#type' =>'textarea',
      '#cols' => 20,
      '#rows' => 20,
      '#default_value' => variable_get('os_convert_map', OS_DEFAULT_MAP),
      '#description' => t('Use in form of "key|value" (no quotes), where key is old agency id (OS 2.x) and value is the new agency id (OS 3.x)'),
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

    /*$form['os_convert_actions']['os_convert_reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
      '#submit' => array('os_convert_admin_form_reset'),
    );*/

    $form['os_convert_warning'] = array(
      '#type' => 'item',
      '#markup' => t('<em>Better backup your database first! Also make sure the php\'s max_execution and memory_limit values are set to adequate values (e.g. 300/512M)</em>'),
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
/*function os_convert_admin_form_reset(&$form, &$form_state) {
  drupal_set_message(t('Counters resetted.'));
  variable_set('os_convert_progress', 0);
  variable_set('os_convert_remapped', FALSE);

  db_query('TRUNCATE `ting_object_convert`');
  db_query('TRUNCATE `ting_object_leftover`');
}*/

function os_convert_admin_form_remap(&$form, &$form_state) {
  os_convert_update_left_over();
  $left_over = os_convert_leftover_count(TRUE);

  $input = $form_state['input'];
  $map = $input['os_convert_map'];
  $array_map = os_convert_process_map($map);
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
      $tid = $row->tid;
      $ding_entity_id = urldecode($row->ding_entity_id);
      list($agency, $local_id) = explode(':', $ding_entity_id);

      if (isset($array_map[$agency])) {
        $run_query = TRUE;
        $new_agency = $array_map[$agency];
        $new_id = $new_agency . ':' . $local_id;
        // Used in WHERE clause.
        $tids[] = $tid;
        $all_tids[] = $tid;
        // Used in query placeholders.
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
  $_tids = array();
  $i = 0;
  foreach ($all_tids as $_tid) {
    $_tids[] = $_tid;

    if ($i % OS_CONVERT_STEP == 0) {
      $tids_string = implode(',', $_tids);
      $_tids = array();
      db_query($cleanup_query . '(' . $tids_string . ')');
    }

    $i++;
  }

  variable_set('os_convert_remapped', TRUE);
  os_convert_processed_count(TRUE);
  $form_state['rebuild'] = TRUE;
}

/**
 * AJAX submit callback for remap button.
 *
 * Simply update the buttons.
 *
 * @see os_convert_admin_form
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
  $map_rows = preg_split ('/$\R?^/m', $map);
  $array_map = array();

  foreach ($map_rows as $row) {
    $r = trim($row);
    $r = explode('|', $r);
    $array_map[$r[0]] = $r[1];
  }

  return $array_map;
}
