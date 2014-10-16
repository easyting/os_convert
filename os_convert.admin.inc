<?php
/**
 * @file
 *
 * Administration related items.
 */

/**
 * Admin form, tiny interface for conversion progress.
 */
function os_convert_admin_form($form, &$form_state) {
  $count = os_convert_objects_count();
  if ($count == 0) {
    variable_set('os_convert_progress', 0);
  }
  $processed = variable_get('os_convert_progress', 0);
  $t = array(
    '@processed' => $processed,
    '@count' => $count,
  );

  if (!isset($form_state['os_convert_init'])) {
    $form['os_convert_count'] = array(
      '#type' => 'item',
      '#markup' => t('Number of cached objects: @count', $t),
    );

    $form['os_convert_progress'] = array(
      '#type' => 'item',
      '#markup' => t('Overall progress: @processed/@count', $t),
    );

    $form['os_convert_run'] = array(
      '#type' => 'submit',
      '#value' => ($t['@processed'] > 0) ? t('Continue conversion') : t('Start conversion'),
      '#disabled' => ($t['@processed'] == $t['@count']) ? TRUE : FALSE,
    );

    $form['os_convert_reset'] = array(
      '#type' => 'submit',
      '#value' => t('Reset'),
      '#submit' => array('os_convert_admin_form_reset'),
      '#disabled' => ($t['@processed'] == 0) ? TRUE : FALSE,
    );

    $form['os_convert_warning'] = array(
      '#type' => 'item',
      '#markup' => t('<em>Better backup your database first!</em>'),
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
    $form['#attached']['css'][] = drupal_get_path('module', 'os_convert') . '/css/os_convert.css';
  }

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
function os_convert_admin_form_reset(&$form, &$form_state) {
  drupal_set_message(t('Counters resetted.'));
  variable_set('os_convert_progress', 0);
}
