(function () {
  var _old_initialize = L.Map.prototype.initialize;

  L.Map.include({

    initialize: function(/*HTMLElement or String*/ id, /*Object*/ options) {
      // alert("loaded");
      // this.on('load', onMapLoad, this);
      _old_initialize.apply(this, [id, options]);
    }
  });

}());
