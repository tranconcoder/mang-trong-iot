document.addEventListener("DOMContentLoaded", () => {
  // MQTT Configuration
  const mqttBroker = "wss://broker.emqx.io:8084/mqtt";
  const mqttTopic = "tranvancon/mangtrongiot";
  const clientId = "dashboard_" + Math.random().toString(16).substring(2, 8);

  // Chart configuration
  const maxDataPoints = 20;
  const temperatureData = {
    labels: [],
    datasets: [
      {
        label: "Temperature (°C)",
        data: [],
        borderColor: "rgba(255, 99, 132, 1)",
        backgroundColor: "rgba(255, 99, 132, 0.2)",
        tension: 0.3,
        fill: true,
      },
    ],
  };

  const humidityData = {
    labels: [],
    datasets: [
      {
        label: "Humidity (%)",
        data: [],
        borderColor: "rgba(54, 162, 235, 1)",
        backgroundColor: "rgba(54, 162, 235, 0.2)",
        tension: 0.3,
        fill: true,
      },
    ],
  };

  const lightData = {
    labels: [],
    datasets: [
      {
        label: "Light Level",
        data: [],
        borderColor: "rgba(75, 192, 192, 1)",
        backgroundColor: "rgba(75, 192, 192, 0.2)",
        tension: 0.3,
        fill: true,
      },
    ],
  };

  // Chart options
  const chartOptions = {
    responsive: true,
    maintainAspectRatio: false,
    scales: { y: { beginAtZero: false } },
    animation: { duration: 500 },
  };

  // Initialize charts
  const tempChart = new Chart(
    document.getElementById("temperatureChart").getContext("2d"),
    { type: "line", data: temperatureData, options: chartOptions }
  );

  const humidityChart = new Chart(
    document.getElementById("humidityChart").getContext("2d"),
    { type: "line", data: humidityData, options: chartOptions }
  );

  const lightChart = new Chart(
    document.getElementById("lightChart").getContext("2d"),
    { type: "line", data: lightData, options: chartOptions }
  );

  // Connect to MQTT broker
  const mqttClient = mqtt.connect(mqttBroker, {
    clientId: clientId,
    clean: true,
    connectTimeout: 4000,
    reconnectPeriod: 1000,
  });

  // MQTT event handlers
  mqttClient.on("connect", () => {
    console.log("Connected to MQTT broker");
    document.getElementById("mqtt-connection-status").textContent =
      "MQTT: Connected";
    document
      .getElementById("mqtt-connection-status")
      .classList.add("connected");

    // Subscribe to topic
    mqttClient.subscribe(mqttTopic, (err) => {
      if (!err) {
        console.log(`Subscribed to ${mqttTopic}`);
      } else {
        console.error("Subscription error:", err);
      }
    });
  });

  mqttClient.on("error", (err) => {
    console.error("MQTT error:", err);
    document.getElementById("mqtt-connection-status").textContent =
      "MQTT: Error";
    document
      .getElementById("mqtt-connection-status")
      .classList.remove("connected");
    document.getElementById("mqtt-connection-status").classList.add("error");
  });

  mqttClient.on("disconnect", () => {
    console.log("Disconnected from MQTT broker");
    document.getElementById("mqtt-connection-status").textContent =
      "MQTT: Disconnected";
    document
      .getElementById("mqtt-connection-status")
      .classList.remove("connected");
  });

  // Handle incoming messages
  mqttClient.on("message", (topic, message) => {
    if (topic === mqttTopic) {
      try {
        const payload = JSON.parse(message.toString());
        console.log("Received message:", payload);

        // Update timestamp and add as label
        const timestamp = new Date();
        const timeLabel = timestamp.toLocaleTimeString();

        // Extract sensor data
        const temperature = payload.sensors.temperature;
        const humidity = payload.sensors.humidity;
        const light = payload.sensors.light;

        // Update UI elements
        document.getElementById("dht22-temp").textContent = `${temperature} °C`;
        document.getElementById("dht22-humidity").textContent = `${humidity} %`;
        document.getElementById("ldr-value").textContent = light;
        document.getElementById("last-update").textContent = timeLabel;
        document.getElementById("device-status").textContent = payload.status;

        // Add data to charts
        if (temperatureData.labels.length >= maxDataPoints) {
          temperatureData.labels.shift();
          temperatureData.datasets[0].data.shift();
          humidityData.labels.shift();
          humidityData.datasets[0].data.shift();
          lightData.labels.shift();
          lightData.datasets[0].data.shift();
        }

        temperatureData.labels.push(timeLabel);
        temperatureData.datasets[0].data.push(temperature);
        humidityData.labels.push(timeLabel);
        humidityData.datasets[0].data.push(humidity);
        lightData.labels.push(timeLabel);
        lightData.datasets[0].data.push(light);

        // Update charts
        tempChart.update();
        humidityChart.update();
        lightChart.update();
      } catch (e) {
        console.error("Error parsing message:", e);
      }
    }
  });

  // LED Control
  const ledButton = document.getElementById("toggleLedButton");
  const ledStatusEl = document.getElementById("led-status");
  let isLedOn = false;

  if (ledButton && ledStatusEl) {
    ledButton.addEventListener("click", () => {
      isLedOn = !isLedOn;
      ledStatusEl.textContent = isLedOn ? "ON" : "OFF";
      ledStatusEl.style.color = isLedOn
        ? "var(--success-color, #2ecc71)"
        : "var(--error-color, #e74c3c)";

      // Publish LED command to MQTT topic
      if (mqttClient.connected) {
        const ledCommand = JSON.stringify({
          device: "dashboard",
          command: "led",
          state: isLedOn ? "on" : "off",
        });
        mqttClient.publish(`${mqttTopic}/command`, ledCommand);
        console.log("LED command published:", ledCommand);
      }
    });
  }
});
