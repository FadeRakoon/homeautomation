<template>
  <v-container fluid>
    <!-- Row 1: Date Inputs and Average Card -->
    <v-row class="row1">
      <v-col cols="4">
        <v-sheet class="sheet pa-4">
          <v-text-field
            v-model="start"
            label="Start date"
            type="date"
            variant="outlined"
            density="comfortable"
            class="mr-5 mb-3"
            :style="{ maxWidth: '300px' }"
            hide-details
          ></v-text-field>

          <v-text-field
            v-model="end"
            label="End date"
            type="date"
            variant="outlined"
            density="comfortable"
            class="mr-5 mb-4"
            :style="{ maxWidth: '300px' }"
            hide-details
          ></v-text-field>

          <!-- Error / Status messages -->
          <div v-if="errorText" class="text-caption text-red mb-2">
            {{ errorText }}
          </div>
          <div v-if="statusText" class="text-caption text-medium-emphasis mb-2">
            {{ statusText }}
          </div>

          <v-btn
            class="text-none mb-4"
            color="primary"
            variant="elevated"
            :loading="loading"
            @click="runAnalysis"
          >Submit
          </v-btn>
        </v-sheet>
      </v-col>

      <!-- Average Card -->
      <v-col cols="3" align="center">
        <v-card
          title="Average"
          subtitle="For the selected period"
          width="450"
          height="200"
          variant="elevated"
          color="surface"
          density="compact"
          rounded="lg"
          align="center"
        >
          <v-card-item align="center">
            <span class="text-h1 text-green" style="white-space: nowrap;">
              {{ avg.value }}
            </span>
            <small>Gal</small>
          </v-card-item>
        </v-card>
      </v-col>
    </v-row>

    <!-- Row 2: Charts -->
    <v-row class="row">
      <v-col cols="12">
        <figure class="highcharts-figure">
          <div id="container"></div>
        </figure>
      </v-col>
      <v-col cols="12">
        <figure class="highcharts-figure">
          <div id="container0"></div>
        </figure>
      </v-col>
    </v-row>
  </v-container>
</template>

<script setup>
// IMPORTS
import { ref, reactive, onMounted, onBeforeUnmount } from "vue";
import { useMqttStore } from "@/store/mqttStore";
import Highcharts from "highcharts";
import more from "highcharts/highcharts-more";
import Exporting from "highcharts/modules/exporting";

more(Highcharts);
Exporting(Highcharts);

// STORES
const Mqtt = useMqttStore();

// VARIABLES
const waterMngAnal = ref(null);
const heightWaterLvl = ref(null);
const loading = ref(false);
const errorText = ref("");
const statusText = ref("");

const start = ref(null);
const end = ref(null);

var avg = reactive({ value: 0 });

// DIRECT API FETCH HELPER
const fetchApiJson = async (url) => {
  try {
    const response = await fetch(url);
    if (!response.ok) return { ok: false, payload: null };
    const payload = await response.json().catch(() => null);
    return { ok: true, payload };
  } catch (error) {
    return { ok: false, payload: null };
  }
};

// CREATE CHARTS
const CreateCharts = () => {
  waterMngAnal.value = Highcharts.chart("container", {
    chart: { zoomType: "x" },
    title: { text: "Water Management Analysis", align: "left" },
    xAxis: {
      type: "datetime",
      title: { text: "Time", style: { color: "#000000" } },
    },
    yAxis: {
      title: { text: "Water Reserve", style: { color: "#000000" } },
      labels: { format: "{value} Gal" },
    },
    tooltip: { shared: true },
    series: [
      {
        name: "Reserve",
        type: "line",
        data: [],
        turboThreshold: 0,
        color: Highcharts.getOptions().colors[0],
      },
    ],
  });

  heightWaterLvl.value = Highcharts.chart("container0", {
    chart: { type: "scatter", zoomType: "xy" },
    title: {
      text: "Height and Water Level Correlation Analysis",
      align: "left",
    },
    xAxis: {
      title: { text: "Water Height (in)", style: { color: "#000000" } },
      labels: { format: "{value} in" },
    },
    yAxis: {
      min: 0,
      title: { text: "Radar Distance (in)", style: { color: "#000000" } },
      labels: { format: "{value} in" },
    },
    series: [
      {
        name: "Analysis",
        type: "scatter",
        data: [],
        turboThreshold: 0,
      },
    ],
  });
};

// UPDATE CHARTS
const updateLineCharts = (data) => {
  if (!data || data.length === 0) {
    statusText.value = "No records found for selected range.";
    waterMngAnal.value.series[0].setData([], true);
    heightWaterLvl.value.series[0].setData([], true);
    return;
  }

  const reserve = [];
  const waterheight = [];

  data.forEach((row) => {
    const tsMs = row.timestamp < 1e12 ? row.timestamp * 1000 : row.timestamp;

    reserve.push([tsMs, Number(row.reserve.toFixed(2))]);

    if (row.waterheight != null && row.radar != null) {
      waterheight.push([
        Number(row.waterheight.toFixed(2)),
        Number(row.radar.toFixed(2)),
      ]);
    }
  });

  waterMngAnal.value.series[0].setData(reserve, true);
  heightWaterLvl.value.series[0].setData(waterheight, true);

  statusText.value = `${data.length} records loaded.`;
};

const runAnalysis = async () => {
  errorText.value = "";
  statusText.value = "";

  if (!start.value || !end.value) {
    errorText.value = "Please provide a valid start and end date.";
    return;
  }

  const startDate = new Date(`${start.value}T00:00:00`).getTime() / 1000;
  const endDate = new Date(`${end.value}T23:59:59`).getTime() / 1000;

  if (startDate > endDate) {
    errorText.value = "Start date cannot be after end date.";
    return;
  }

  loading.value = true;
  statusText.value = "Loading data...";

  try {
    const [reserveResult, avgResult] = await Promise.all([
      fetchApiJson(`/api/reserve/${startDate}/${endDate}`),
      fetchApiJson(`/api/avg/${startDate}/${endDate}`),
    ]);

    // Handle reserve data
    if (reserveResult.ok && reserveResult.payload?.status === "found") {
      updateLineCharts(reserveResult.payload.data);
    } else {
      updateLineCharts([]);
    }

    if (avgResult.ok && avgResult.payload?.status === "found") {
      const raw = avgResult.payload.data;
      const parsed =
        typeof raw === "number"
          ? raw
          : Number(raw?.average ?? raw?.avg ?? raw ?? 0);
      avg.value = Number.isFinite(parsed) ? parsed.toFixed(1) * 1 : 0;
    } else {
      avg.value = 0;
    }
  } catch (e) {
    errorText.value = "Failed to load data. Check your backend connection.";
    console.error("runAnalysis error:", e);
  } finally {
    loading.value = false;
  }
};

onMounted(() => {
  Mqtt.connect();
  setTimeout(() => {
    Mqtt.subscribe("620171573");
  }, 3000);

  CreateCharts();
});

onBeforeUnmount(() => {
  Mqtt.unsubscribeAll();

  if (waterMngAnal.value) {
    waterMngAnal.value.destroy();
    waterMngAnal.value = null;
  }
  if (heightWaterLvl.value) {
    heightWaterLvl.value.destroy();
    heightWaterLvl.value = null;
  }
});
</script>

<style scoped>
.highcharts-figure {
  border: 2px solid black;
}
</style>