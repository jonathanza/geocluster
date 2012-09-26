(function ($) {

  Drupal.behaviors.geocluster_test = {

    onMapLoad: function(event) {
      var map = this;
      var center = map.getCenter();
      var bounds = map.getBounds();

      var sw = bounds.getSouthWest();
      var ne = bounds.getNorthEast();

      var height = ne.lat - sw.lat;
      var width = ne.lng - sw.lng;

      var length = Geohash.get_key_length(height, width);
      var hash = Geohash.get_key(center.lat, center.lng, length);
      var bounds_geohash = Geohash.bbox(hash);

      length += 1;
      var hash = Geohash.get_key(center.lat, center.lng, length);

      // convert bounds
      var bounds = [[bounds_geohash[3], bounds_geohash[0]], [bounds_geohash[1], bounds_geohash[2]]];

      // create an orange rectangle
      L.rectangle(bounds, {color: "#ff7800", weight: 1}).addTo(map);

      // zoom the map to the rectangle bounds
      // map.fitBounds(bounds);

      length += 1;
      var hash = Geohash.get_key(center.lat, center.lng, length);

      Drupal.behaviors.geocluster_test.hashChildren(hash, map, bounds_geohash);
    },


    drawHashRect: function(hash, map) {
      var bounds_geohash = Geohash.bbox(hash);
      // convert bounds
      var bounds = [[bounds_geohash[3], bounds_geohash[0]], [bounds_geohash[1], bounds_geohash[2]]];

      // create an orange rectangle
      L.rectangle(bounds, {color: "#ff7800", weight: 1}).addTo(map);
    },

    hashChildren: function(hash, map, bounds_geohash) {
      var keys = new Object;
      Geohash.fill_bbox(hash, keys, bounds_geohash);
      $.each(keys, function(key) {
        var bounds_geohash = Geohash.bbox(key);
        // convert bounds
        var bounds = [[bounds_geohash[3], bounds_geohash[0]], [bounds_geohash[1], bounds_geohash[2]]];

        L.rectangle(bounds, {color: "#ff7800", weight: 1}).addTo(map);
      });
    }
  };

  var _old_initialize = L.Map.prototype.initialize;

  L.Map.include({

    initialize: function(/*HTMLElement or String*/ id, /*Object*/ options) {
      _old_initialize.apply(this, [id, options]);
      this.on('load', Drupal.behaviors.geocluster_test.onMapLoad, this);
    }

  });

})(jQuery);
