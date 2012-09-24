<?php

/**
 * @file
 * Geocluster extension for geofield and maps.
 */

class GeoclusterHelper {

  /**
   * Resolutions indexed by zoom levels.
   *
   * The resolutions are in meters / pixel, so the most common use is to divide
   * the distance between points by the resolution in order to determine the
   * number of pixels between the features.
   *
   * @return array An array of resolutions indexed by zoom levels.
   */
  static function resolutions() {
    // see https://github.com/mapbox/clustr/blob/gh-pages/src/clustr.js#L4
    $r = array();
    // Meters per pixel.
    $maxResolution = 156543.03390625;
    $maxResolution = GEOFIELD_KILOMETERS * 1000 / 256;
    // see http://wiki.openstreetmap.org/wiki/Zoom_levels
    $maxResolution = 156.412 * 1000;
    for($zoom = 0; $zoom <= 30; ++$zoom) {
        $r[$zoom] = $maxResolution / pow(2, $zoom);
    }
    return $r;
  }

  /**
   * Dumb implementation to incorporate pixel variation with latitude
   * on the mercator projection.
   *
   * @todo: use a valid implementation instead of the current guessing.
   *
   * Current implementation is based on the observation:
   * lat = 0 => output is correct
   * lat = 48 => output is 223 pixels distance instead of 335 in reality.
   *
   * @param $lat
   * @return float the correction factor
   */
  static function pixel_correction($lat) {
    return 1 + (335.0 / 223.271875276 - 1) * ((float)(abs($lat)) / 47.9899);
  }

  /**
   * Calculate the distance in pixels between two given points.
   *
   * This depends on the resolution (zoom level) they are viewed in.
   *
   * @param $geometry
   * @param $otherGeometry
   * @param $resolution
   * @return float
   */
  static function distance_pixels($geometry, $otherGeometry, $resolution) {
    $distance = GeoclusterHelper::distance_haversine($geometry, $otherGeometry);
    $distance_pixels = $distance / $resolution * 1000 * GeoclusterHelper::pixel_correction($geometry->getY());
    return $distance_pixels;
  }
  // see http://www.codecodex.com/wiki/Calculate_Distance_Between_Two_Points_on_a_Globe#PHP
  function distance_haversine($geometry, $otherGeometry) {
    $long_1 = (float) $geometry->getX();
    $lat_1 = (float) $geometry->getY();
    $long_2 = (float) $otherGeometry->getX();
    $lat_2 = (float) $otherGeometry->getY();

    $earth_radius = GEOFIELD_KILOMETERS; // in km

    $dLat = deg2rad($lat_2 - $lat_1);
    $dLon = deg2rad($long_2 - $long_1);

    $a = sin($dLat/2) * sin($dLat/2) + cos(deg2rad($lat_1)) * cos(deg2rad($lat_2)) * sin($dLon/2) * sin($dLon/2);
    $c = 2 * asin(sqrt($a));
    $d = $earth_radius * $c;

    return $d;
  }

  static function forward_mercator($lat, $lon) {
    $x = $lon * 20037508.34 / 180;
    $y =log(tan((90 + $lat) * pi() / 360)) / (pi() / 180);
    $y = $y * 20037508.34 / 180;
    return array('x' => $x, 'y' => $y);
  }

  public static function geohash_length_from_zoom_level($zoom) {
    //todo: implement.
    return 4;
  }

  /*
  function geocluster_distance_simple2($geometry, $otherGeometry) {
    $point = geocluster_forward_mercator($geometry->getX(), $geometry->getY());
    $long_1 = $point['x'];
    $lat_1 = $point['y'];

    $point = geocluster_forward_mercator($otherGeometry->getX(), $otherGeometry->getY());
    $long_2 = $point['x'];
    $lat_2 = $point['x'];

    return sqrt(
      pow($long_1 - $long_2, 2) +
        pow($lat_2 - $lat_2, 2)
    );
  }

  function geocluster_distance_simple($geometry, $otherGeometry) {
    $long_1 = $geometry->getX();
    $lat_1 = $geometry->getY();
    $long_2 = $otherGeometry->getX();
    $lat_2 = $otherGeometry->getY();

    return sqrt(
      pow($long_1 - $long_2, 2) +
      pow($lat_2 - $lat_2, 2)
    );
  }

  */

}
