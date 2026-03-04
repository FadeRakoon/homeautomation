<template>
  <v-container>
    <v-row>
      <v-col cols="9" md="1">
        <v-sheet elevation="2" class="pa-4">
          <v-slider
            direction="vertical"
            v-model="radarValue"
            :max="100"
            color="green"
            track-color="green-lighten-3"
            thumb-color="green-darken-2"
            label=" Height(in)"
          ></v-slider>
        </v-sheet>
      </v-col>

      <v-col cols="12" md="11">
        <v-sheet elevation="2" class="pa-4">
          <div id="areaChart" style="height: 400px"></div>
        </v-sheet>
      </v-col>
    </v-row>

    <v-row>
      <v-col cols="12" md="8">
        <v-sheet elevation="2" class="pa-4">
          <div id="gaugeChart" style="height: 300px"></div>
        </v-sheet>
      </v-col>

      <!-- Liquid Progress Indicator -->
      <v-col cols="12" md="4">
        <v-sheet elevation="2" class="pa-4">
          <v-card>
            <div id="fluid-meter"></div>
          </v-card>
        </v-sheet>
      </v-col>
    </v-row>

    <!-- Overflow Dialog -->
    <v-dialog v-model="overflowDialog" max-width="500">
      <v-card>
        <v-card-title class="headline">Overflow Detected</v-card-title>
        <v-card-actions>
          <v-spacer></v-spacer>
          <v-btn color="primary" @click="overflowDialog = false">Close</v-btn>
        </v-card-actions>
      </v-card>
    </v-dialog>
  </v-container>
</template>

<script setup>
/** JAVASCRIPT HERE */

// IMPORTS
import {
  ref,
  reactive,
  watch,
  onMounted,
  onBeforeUnmount,
  computed,
} from "vue";
import { useRoute, useRouter } from "vue-router";
import { useMqttStore } from "@/store/mqttStore"; // Import Mqtt Store
import { storeToRefs } from "pinia";

import Highcharts from "highcharts";
import more from "highcharts/highcharts-more";
import Exporting from "highcharts/modules/exporting";

more(Highcharts);
Exporting(Highcharts);

// VARIABLES
const router = useRouter();
const route = useRoute();
const Mqtt = useMqttStore();
const { payload, payloadTopic } = storeToRefs(Mqtt);
const areaChart = ref(null);
const gaugeChart = ref(null);
const points = ref(600);
const shift = ref(false);
const radarValue = ref(0);
const reserveData = ref([]);
const percentageValue = ref(0);
const overflowDialog = ref(false);

var fm = new FluidMeter();

// FUNCTIONS

const CreateCharts = async () => {
  areaChart.value = Highcharts.chart("areaChart", {
    title: { text: "Water Reserves(10min)", align: "left" },
    chart: { zoomtype: "x" },
    tooltip: { shared: true },
    yAxis: {
      title: { text: "Water Level" },
      labels: { format: "{value} Gal" },
    },
    xAxis: {
      title: { text: "Time" },
      type: "datetime",
    },
    series: [{ name: "Water", type: "area", data: [], color: "#00FFFF" }],
  });

  gaugeChart.value = Highcharts.chart("gaugeChart", {
    title: { text: "Water Reserves", align: "left" },
    // the value axis
    yAxis: {
      min: 0,
      max: 1000,
      tickPixelInterval: 72,
      tickPosition: "inside",
      tickColor: Highcharts.defaultOptions.chart.backgroundColor || "#FFFFFF",
      tickLength: 20,
      tickWidth: 2,
      minorTickInterval: null,
      labels: { distance: 20, style: { fontSize: "14px" } },
      lineWidth: 0,
      plotBands: [
        { from: 0, to: 200, color: "#DF5353", thickness: 20 },
        { from: 200, to: 600, color: "#DDDF0D", thickness: 20 },
        { from: 600, to: 1000, color: "#55BF3B", thickness: 20 },
      ],
    },
    tooltip: { shared: true },
    pane: {
      startAngle: -90,
      endAngle: 89.9,
      background: null,
      center: ["50%", "75%"],
      size: "110%",
    },
    series: [
      {
        type: "gauge",
        name: "Water Capacity",
        data: [0],
        tooltip: { valueSuffix: " Gal" },
        dataLabels: {
          format: "{y} Gal",
          borderWidth: 0,
          color:
            (Highcharts.defaultOptions.title &&
              Highcharts.defaultOptions.title.style &&
              Highcharts.defaultOptions.title.style.color) ||
            "#333333",
          style: { fontSize: "16px" },
        },
        dial: {
          radius: "80%",
          backgroundColor: "gray",
          baseWidth: 12,
          baseLength: "0%",
          rearLength: "0%",
        },
        pivot: { backgroundColor: "gray", radius: 6 },
      },
    ],
  });
  fm.init({
    targetContainer: document.getElementById("fluid-meter"),
    fillPercentage: 15,
    options: {
      fontSize: "70px",
      fontFamily: "Arial",
      fontFillStyle: "white",
      drawShadow: true,
      drawText: true,
      drawPercentageSign: true,
      drawBubbles: true,
      size: 300,
      borderWidth: 25,
      backgroundColor: "#e2e2e2",
      foregroundColor: "#fafafa",
      foregroundFluidLayer: {
        fillStyle: "purple",
        angularSpeed: 100,
        maxAmplitude: 12,
        frequency: 30,
        horizontalSpeed: -150,
      },
      backgroundFluidLayer: {
        fillStyle: "pink",
        angularSpeed: 100,
        maxAmplitude: 9,
        frequency: 30,
        horizontalSpeed: 150,
      },
    },
  });
};

onMounted(() => {
  // THIS FUNCTION IS CALLED AFTER THIS COMPONENT HAS BEEN MOUNTED
  CreateCharts();

  Mqtt.connect(); // Connect to Broker located on the backend
  setTimeout(() => {
    // Subscribe to each topic
    Mqtt.subscribe("620171573");
    //Mqtt.subscribe("topic2");
  }, 3000);
});

onBeforeUnmount(() => {
  // THIS FUNCTION IS CALLED RIGHT BEFORE THIS COMPONENT IS UNMOUNTED
  // unsubscribe from all topics
  Mqtt.unsubscribeAll();
});

// WATCHERS
watch(payload, (data) => {
  if (points.value > 0) {
    points.value--;
  } else {
    shift.value = true;
  }
  radarValue.value = parseFloat(data.radar.toFixed(0));
  areaChart.value.series[0].addPoint(
    { y: parseFloat(data.reserve.toFixed(2)), x: data.timestamp * 1000 },
    true,
    shift.value,
  );
  gaugeChart.value.series[0].points[0].update(
    parseFloat(data.reserve.toFixed(2)),
  );
  fm.setPercentage(parseFloat(data.percentage.toFixed(0)));

  if (data.percentage > 100) {
    overflowDialog.value = true;
  }
});
</script>

<style scoped>
/** CSS STYLE HERE */
</style>