/**
 * @file
 * Run cyclic AJAX commands to keep the convertion process.
 */
(function($) {
  Drupal.behaviors.os_convert_init = {
    attach: function(context) {
      var ele = $('#os-convert-progress');
      var loader = ele.find('.loader').hide();

      $(ele).bind('os_convert_init', function() {
        loader.show();
        $.ajax({
          type: 'post',
          url: '/admin/config/ting/os_convert/progress',
          data: {},
          dataType: 'json',
          success: function(response) {
            ele.find('.form-type-item').find('.progress').html(response.message);
            loader.hide();
            if (parseInt(response.percentage) !== 100) {
              ele.trigger('os_convert_init');
            }
          }
        });
      });

      ele.trigger('os_convert_init');
    }
  };

})(jQuery);
