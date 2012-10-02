(function ($) {

  Drupal.geocluster = {

    map: null,
    markerGroup: null,

    onMapLoad: function(event) {
      var map = this;
      Drupal.geocluster.map = map;

      Drupal.geocluster.markerGroup = new L.LayerGroup();
      Drupal.geocluster.markerGroup.addTo(map);

      map.on('moveend', Drupal.geocluster.moveEnd);
      Drupal.geocluster.moveEnd();
    },

    moveEnd: function(e) {
      var map = Drupal.geocluster.map;
      Drupal.geocluster.makeGeoJSONLayer(map);
    },

    makeGeoJSONLayer: function(map, url) {
      url = typeof url !== 'undefined' ? url : Drupal.settings.basePath + "json-cluster";

      url += "?bbox=" + map.getBounds().pad(0.05).toBBoxString();
      url += "&zoom=" + map.getZoom();

      if (Drupal.geocluster._get['cluster_distance'] != undefined) {
        url += "&cluster_distance=" + Drupal.geocluster._get['cluster_distance'];
      }

      $.getJSON(url, function(data) {
        //New GeoJSON layer
        var geojsonLayer = new L.GeoJSON(data, {
          onEachFeature: function(featureData, layer) {
            var popupText = featureData.properties.name;
            layer.bindPopup(popupText);
          },
          pointToLayer: function(featureData, latlng) {
            var icon = new L.NumberedDivIcon({number: featureData.cluster_items || 1});
            lMarker = new L.Marker(latlng, {icon:icon});
            return lMarker;
          }
        });
        Drupal.geocluster.markerGroup.clearLayers();
        Drupal.geocluster.markerGroup.addLayer(geojsonLayer);
      });
    },

    _get: new (function(){
      var parts = window.location.search.substr(1).split("&");
      var $_GET = {};
      for (var i = 0; i < parts.length; i++) {
        var temp = parts[i].split("=");
        $_GET[decodeURIComponent(temp[0])] = decodeURIComponent(temp[1]);
      }
      return $_GET;
    })
  };

  _geocluster_old_leaflet_initialize = L.Map.prototype.initialize;

  L.Map.include({

    initialize: function(/*HTMLElement or String*/ id, /*Object*/ options) {
      _geocluster_old_leaflet_initialize.apply(this, [id, options]);
      this.on('load', Drupal.geocluster.onMapLoad, this);
    }

  });

})(jQuery);
