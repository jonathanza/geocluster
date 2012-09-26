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

      Drupal.behaviors.geocluster_test.drawHashRect(hash, map, "#00ff00", 0.1);

      length += 1;
      var hash = Geohash.get_key(center.lat, center.lng, length);

      // zoom the map to the rectangle bounds
      // map.fitBounds(bounds);

      var hash = Geohash.get_key(center.lat, center.lng, length);

      Drupal.behaviors.geocluster_test.hashChildren(hash, map, bounds_geohash, "#ff0000", 0.5);


      length += 1;
      var hash = Geohash.get_key(center.lat, center.lng, length);

      Drupal.behaviors.geocluster_test.hashChildren(hash, map, bounds_geohash, "#00ff00", 0.1);
    },

    drawHashRect: function(hash, map, color, opacity) {
      color = typeof color !== 'undefined' ? color : "#ff7800";
      opacity = typeof opacity !== 'undefined' ? opacity : 0.5;

      var bounds_geohash = Geohash.bbox(hash);
      // convert bounds
      var bounds = [[bounds_geohash[3], bounds_geohash[0]], [bounds_geohash[1], bounds_geohash[2]]];

      // create an orange rectangle
      L.rectangle(bounds, {color: color, weight: 1, opacity: opacity, fillOpacity: 0.0}).addTo(map);
    },

    hashChildren: function(hash, map, bounds_geohash, color, opacity) {
      var keys = new Object;
      Geohash.fill_bbox(hash, keys, bounds_geohash);
      $.each(keys, function(key) {
        Drupal.behaviors.geocluster_test.drawHashRect(key, map, color, opacity);
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
