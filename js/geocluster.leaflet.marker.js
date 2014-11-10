(function ($) {

  // Numbered Markers in Leaflet
  // based on https://gist.github.com/2288108

  L.NumberedDivIcon = L.Icon.extend({
    options: {
      iconUrl: Drupal.settings.basePath + 'sites/all/modules/geocluster/images/marker_hole.png',
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
      this.options['iconUrl'] = Drupal.settings.basePath + 'sites/all/modules/geocluster/images/marker_hole.png';

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

  Drupal.leaflet.create_point = function(marker, lMap) {
    // var point = Drupal.leaflet._old_create_point(marker);
    // point.options.icon.options.number = marker.geocluster_count || 10;
    // return point;

    // If this is a clustered marker adjust some settings.
    if (marker.geocluster_count > 1) {
      marker.html = '<div><span>' + marker.geocluster_count + '</span></div>';
      marker.html_class =  'marker-cluster' + c;
    }

    var lMarker = Drupal.leaflet._old_create_point(marker, lMap);

    // If this is a clustered marker inject our own icon.
    if (marker.geocluster_count > 1) {
      // var icon = new L.NumberedDivIcon({number: marker.geocluster_count || 1});

      var c = ' marker-cluster-';
      if (marker.geocluster_count < 10) {
        c += 'small';
      } else if (marker.geocluster_count < 100) {
        c += 'medium';
      } else {
        c += 'large';
      }
      var icon = new L.DivIcon({ html: '<div><span>' + marker.geocluster_count + '</span></div>', className: 'marker-cluster' + c, iconSize: new L.Point(40, 40) });
      lMarker.setIcon(icon)
    }
    return lMarker;
  }

})(jQuery);
