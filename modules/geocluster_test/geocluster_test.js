(function ($) {

  Drupal.behaviors.geocluster_test = {

    url: "http://localhost/mapping/json-cluster",
    map: null,
    layerGroup: null,

    _get: new (function(){
      var parts = window.location.search.substr(1).split("&");
      var $_GET = {};
      for (var i = 0; i < parts.length; i++) {
        var temp = parts[i].split("=");
        $_GET[decodeURIComponent(temp[0])] = decodeURIComponent(temp[1]);
      }
      return $_GET;
    }),

    moveEnd: function(e) {
      var map = Drupal.behaviors.geocluster_test.map;

      var url = Drupal.behaviors.geocluster_test.url;
      url += "?bbox=" + map.getBounds().toBBoxString();
      url += "&zoom=" + map.getZoom();

      if (Drupal.behaviors.geocluster_test._get['cluster_distance'] != undefined) {
        url += "&cluster_distance=" + Drupal.behaviors.geocluster_test._get['cluster_distance'];
      }

      $.getJSON(url, function(data) {
        var geojsonLayer = new L.GeoJSON(data);		//New GeoJSON layer
        Drupal.behaviors.geocluster_test.layerGroup.clearLayers();
        Drupal.behaviors.geocluster_test.layerGroup.addLayer(geojsonLayer);
      });
    },

    loadGeoJSON: function(map) {
      $.getJSON(Drupal.behaviors.geocluster_test.url, function (data) {
        //When GeoJSON is loaded
        var geojsonLayer = new L.GeoJSON(data);		//New GeoJSON layer
        layerGroup = new L.LayerGroup();
        layerGroup.addLayer(geojsonLayer);
        layerGroup.addTo(map);
        Drupal.behaviors.geocluster_test.layerGroup = layerGroup;
      });
      Drupal.behaviors.geocluster_test.map = map;
      map.on('moveend', Drupal.behaviors.geocluster_test.moveEnd);
    },

    onMapLoad: function(event) {
      var map = this;

      Drupal.behaviors.geocluster_test.loadGeoJSON(map);

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

  // Numbered Markers in Leaflet
  // based on https://gist.github.com/2288108

  L.NumberedDivIcon = L.Icon.extend({
    options: {
      iconUrl: Drupal.settings.basePath + 'sites/all/modules/geocluster/modules/geocluster_test/images/marker_hole.png',
      number: '',
      shadowUrl: null,
      iconSize: new L.Point(25, 41),
      iconAnchor: new L.Point(13, 41),
      popupAnchor: new L.Point(0, -33),
      /*
       iconAnchor: (Point)
       popupAnchor: (Point)
       */
      className: 'leaflet-div-icon'
    },

    createIcon: function () {
      this.options['iconUrl'] = Drupal.settings.basePath + 'sites/all/modules/geocluster/modules/geocluster_test/images/marker_hole.png';

      var div = document.createElement('div');
      var img = this._createImg(this.options['iconUrl']);
      var numdiv = document.createElement('div');
      numdiv.setAttribute ( "class", "number" );
      numdiv.innerHTML = this.options['number'] || '';
      div.appendChild ( img );
      div.appendChild ( numdiv );
      this._setIconStyles(div, 'icon');
      return div;
    },

    //you could change this to add a shadow like in the normal marker if you really wanted
    createShadow: function () {
      return null;
    }
  });

  Drupal.leaflet._old_create_point = Drupal.leaflet.create_point;

  Drupal.leaflet.create_point = function(marker) {
    // var point = Drupal.leaflet._old_create_point(marker);
    // point.options.icon.options.number = marker.cluster_items || 10;
    // return point;

    var latLng = new L.LatLng(marker.lat, marker.lon);
    this.bounds.push(latLng);
    var lMarker;

    if (marker.clustered) {
      marker.icon = Object();
      marker.icon.clustered = true;
    }

    if (marker.icon) {
      if (marker.icon.clustered) {
        var icon = new L.NumberedDivIcon({number: marker.cluster_items || 1});
      }
      else {
        var icon = new L.Icon({iconUrl: marker.icon.iconUrl});
      }
      // override applicable marker defaults
      if (marker.icon.iconSize) {
        icon.options.iconSize = new L.Point(parseInt(marker.icon.iconSize.x), parseInt(marker.icon.iconSize.y));
      }
      if (marker.icon.iconAnchor) {
        icon.options.iconAnchor = new L.Point(parseFloat(marker.icon.iconAnchor.x), parseFloat(marker.icon.iconAnchor.y));
      }
      if (marker.icon.popupAnchor) {
        icon.options.popupAnchor = new L.Point(parseFloat(marker.icon.popupAnchor.x), parseFloat(marker.icon.popupAnchor.y));
      }
      if (marker.icon.shadowUrl !== undefined) {
        icon.options.shadowUrl = marker.icon.shadowUrl;
      }
      if (marker.icon.shadowSize) {
        icon.options.shadowSize = new L.Point(parseInt(marker.icon.shadowSize.x), parseInt(marker.icon.shadowSize.y));
      }

      lMarker = new L.Marker(latLng, {icon:icon});
    }
    else {
      lMarker = new L.Marker(latLng);
    }
    return lMarker;
  }
})(jQuery);
