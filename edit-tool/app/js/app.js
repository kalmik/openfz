/**
 * Created by sergiofilipe on 9/25/16.
 */

(function ($) {
  var $openfile = $('#openfile'),
      $listRecentFiles = $openfile.closest(".dropdown").find('.panel-body');

  var showFuzzy = function (data) {

  };

  var loadfile = function (event) {
    var $target = $(event.target).closest('div'),
        name = $target.data('name');

    $.get('http://localhost:5000/'+name, function (response) {
      showFuzzy(response);
    });
  };

  $.get('http://localhost:5000/list', function (response) {
    console.log(response)
    response.forEach(function (el) {
      var $button = $("<div>")
        .attr({'data-name': el})
        .append(
          $("<a>").css({cursor: 'pointer' })
            .append($("<i>").addClass('glyphicon glyphicon-file').text(el))
      ).click(loadfile);
      $listRecentFiles.append($button);
    });
  })
})(jQuery);