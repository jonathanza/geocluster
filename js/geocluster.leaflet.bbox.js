(function ($) {

  // Replace the default options for our use case.
  Drupal.leafletBBox.geoJSONOptions = {

    pointToLayer: function(featureData, latlng) {
      if (featureData.cluster_items > 1) {
        var icon = new L.NumberedDivIcon({number: featureData.cluster_items || 1});
        lMarker = new L.Marker(latlng, {icon:icon});
      }
      else {
        lMarker = new L.Marker(latlng);
      }
      return lMarker;
    },

    onEachFeature: function(featureData, layer) {
      var popupText = featureData.properties.name;

      if (featureData.cluster_items > 1) {
        layer.on('click', function(e) {
          Drupal.leafletBBox.geoJSONOptions.clickOnClustered(e, featureData, layer);
        });
      }
      else {
        layer.bindPopup(popupText);
      }
    },

    clickOnClustered: function(e, featureData, layer) {
        var map = layer._map;
        // Close any other opened popup.
        if (map._popup) {
          map._popup._source.closePopup();
        }
        // Zoom and pan to clicked item.
        map.panTo(layer.getLatLng());
        map.zoomIn();
    }

  };

})(jQuery);
