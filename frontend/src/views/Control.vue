<template>
  <VContainer class="fill-height" align="center" justify="center">
    <v-card
      class="py-8 px-6 text-center mx-auto ma-4"
      elevation="12"
      max-width="400"
      width="100%"
    >
      <h3 class="text-h6 mb-4">Combination</h3>
      <div class="text-body-2">Enter Your 4-Digit Passcode</div>
      <v-sheet color="surface">
        <v-otp-input
          v-model="passcode"
          type="number"
          :length="4"
          autofocus
        ></v-otp-input>
      </v-sheet>
      <v-btn
        class="my-4"
        color="red"
        height="40"
        text="Submit"
        variant="flat"
        width="70%"
        @click="setPasscode()"
      ></v-btn>
    </v-card>
  </VContainer>
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
import { useAppStore } from "@/store/appStore";
import { useMqttStore } from "@/store/mqttStore"; // Import Mqtt Store
import { storeToRefs } from "pinia";

// VARIABLES
const router = useRouter();
const route = useRoute();
const AppStore = useAppStore();
const passcode = ref("");
const Mqtt = useMqttStore();
const { payload, payloadTopic } = storeToRefs(Mqtt);

// FUNCTIONS
const setPasscode = async () => {
  await AppStore.setCombination(passcode.value);
};

onMounted(() => {
  // THIS FUNCTION IS CALLED AFTER THIS COMPONENT HAS BEEN MOUNTED
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
</script>

<style scoped>
/** CSS STYLE HERE */
</style>