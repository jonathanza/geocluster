<?php

/**
 * MySQL aggregation based clustering for geocluster.
 */
class MySQLGeohashGeoclusterAlgorithm2 extends MySQLGeohashGeoclusterAlgorithm {

  function __construct($config, $cluster_distance, $zoom, $field_handler) {
    parent::__construct($config, $cluster_distance, $zoom, $field_handler);
  }

  /**
   * Use default neighbor check.
   */
  protected function clusterByNeighborCheck(&$results_by_geohash) {
    uasort($results_by_geohash, function($a, $b) {
      return GeohashGeoclusterAlgorithm::sortResultsByCount($a, $b);
    });

    foreach ($results_by_geohash as $current_hash => &$results) {
      if (empty($current_hash)) {
        continue;
      }
      $item_key = current(array_keys($results));
      $item = $results[$item_key];
      // Check top right neighbor hashes for overlapping points.
      // Top-right is enough because by the way geohash is structured,
      // future geohashes are always top, topright or right
      $hash_stack = GeohashHelper::getNeighbors($current_hash);
      foreach ($hash_stack as $hash) {
        if (isset($results_by_geohash[$hash])) {
          $other_item_key = current(array_keys($results_by_geohash[$hash]));
          $other_item = $results[$other_item_key];
          $geometry = $this->getGeometry($this->values[$item_key]);
          $other_geometry = $this->getGeometry($this->values[$other_item_key]);

          $size = $item->geocluster_count + $other_item->geocluster_count;
          if ($this->shouldCluster($geometry, $other_geometry, $size)) {
            $this->addCluster($item_key, $other_item_key, $current_hash, $hash, $results_by_geohash);
            if (!isset($results_by_geohash[$current_hash])) {
              continue 2;
            }
          }
        }
      }
    }
  }


  /**
   * Determine if two geofields should be clustered as of their distance.
   */
  protected function shouldCluster($geometry, $otherGeometry, $size) {
    // Calculate distance.
    $distance = GeoclusterHelper::distance_pixels($geometry, $otherGeometry, $this->resolution);
    $distance = $distance - ($size * 1);
    return $distance <= $this->cluster_distance;
  }

  /**
   * Use default finalization.
   */
  protected function finalizeClusters() {
    parent::finalizeClusters();
  }

}


