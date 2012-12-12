(function ($) {

  // Replace the default options for our use case.
  Drupal.leafletBBox.geoJSONOptions = {

    onEachFeature: function(featureData, layer) {
      var popupText = featureData.properties.name;
      layer.bindPopup(popupText);
    },

    pointToLayer: function(featureData, latlng) {
      var icon = new L.NumberedDivIcon({number: featureData.cluster_items || 1});
      lMarker = new L.Marker(latlng, {icon:icon});
      return lMarker;
    }

  };

})(jQuery);
