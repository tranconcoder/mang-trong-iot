document.addEventListener("DOMContentLoaded", () => {
  // MQTT Configuration
  const mqttBroker = "wss://broker.emqx.io:8084/mqtt";
  const mqttTopic = "22004015/tranvancon";
  const ledControlTopic = "22004015/tranvancon/led";
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
    scales: { 
      y: { beginAtZero: false },
      x: { 
        display: true,
        title: {
          display: true,
          text: 'Time'
        }
      }
    },
    animation: { duration: 500 },
    plugins: {
      legend: {
        display: true,
        position: 'top'
      },
      title: {
        display: true,
        text: 'Last 1 Minute History'
      }
    }
  };

  // Initialize charts
  const tempChart = new Chart(
    document.getElementById("temperatureChart").getContext("2d"),
    { type: "line", data: temperatureData, options: { ...chartOptions, plugins: { ...chartOptions.plugins, title: { ...chartOptions.plugins.title, text: 'Temperature Chart' } } } }
  );

  const humidityChart = new Chart(
    document.getElementById("humidityChart").getContext("2d"),
    { type: "line", data: humidityData, options: { ...chartOptions, plugins: { ...chartOptions.plugins, title: { ...chartOptions.plugins.title, text: 'Humidity Chart' } } } }
  );

  const lightChart = new Chart(
    document.getElementById("lightChart").getContext("2d"),
    { type: "line", data: lightData, options: { ...chartOptions, plugins: { ...chartOptions.plugins, title: { ...chartOptions.plugins.title, text: 'Light Level Chart' } } } }
  );

  // Authentication helper
  function getAuthToken() {
    return localStorage.getItem("authToken");
  }

  // API helper function
  async function fetchAuthenticatedAPI(url, options = {}) {
    const token = getAuthToken();
    if (!token) {
      console.error("No auth token found");
      return null;
    }

    const defaultOptions = {
      headers: {
        "Authorization": `Bearer ${token}`,
        "Content-Type": "application/json",
        ...options.headers
      }
    };

    try {
      console.log(`Making API request to: ${url}`);
      const response = await fetch(url, { ...options, ...defaultOptions });
      console.log(`API response status: ${response.status}`);
      
      if (response.ok) {
        const data = await response.json();
        console.log(`API response data:`, data);
        return data;
      } else {
        console.error("API request failed:", response.status, response.statusText);
        const errorText = await response.text();
        console.error("Error response:", errorText);
        return null;
      }
    } catch (error) {
      console.error("API request error:", error);
      return null;
    }
  }

  // Load initial dashboard data
  async function loadDashboardData() {
    try {
      console.log("Loading dashboard data...");
      const response = await fetchAuthenticatedAPI("/api/dashboard/data");
      if (response && response.success) {
        const data = response.data;
        
        // Update UI elements
        document.getElementById("dht22-temp").textContent = `${data.sensors.temperature.toFixed(1)} °C`;
        document.getElementById("dht22-humidity").textContent = `${data.sensors.humidity.toFixed(1)} %`;
        document.getElementById("ldr-value").textContent = data.sensors.light;
        document.getElementById("last-update").textContent = new Date(data.lastUpdate).toLocaleTimeString();
        document.getElementById("device-status").textContent = data.status;
        
        // Update LED status
        const ledStatusEl = document.getElementById("led-status");
        if (ledStatusEl) {
          ledStatusEl.textContent = data.led.status;
          ledStatusEl.style.color = data.led.state ? "var(--success-color, #2ecc71)" : "var(--error-color, #e74c3c)";
        }
        
        console.log("Dashboard data loaded successfully:", data);
      } else {
        console.warn("Failed to load dashboard data:", response);
      }
    } catch (error) {
      console.error("Error loading dashboard data:", error);
    }
  }

  // Load chart data for specified time range
  async function loadChartData(minutes = 1) {
    try {
      console.log(`Loading chart data for ${minutes} minutes...`);
      const response = await fetchAuthenticatedAPI(`/api/sensors/chart?minutes=${minutes}`);
      
      if (response && response.success) {
        const chartData = response.data;
        console.log("Chart data received:", chartData);
        
        // Update temperature chart
        if (chartData.dht && chartData.dht.count > 0) {
          console.log(`Updating temperature chart with ${chartData.dht.count} DHT data points`);
          temperatureData.labels = [...chartData.dht.labels];
          temperatureData.datasets[0].data = [...chartData.dht.temperature];
          tempChart.options.plugins.title.text = `Temperature - Last ${minutes} min (${chartData.dht.count} points)`;
          tempChart.update('none');
          
          // Update humidity chart
          humidityData.labels = [...chartData.dht.labels];
          humidityData.datasets[0].data = [...chartData.dht.humidity];
          humidityChart.options.plugins.title.text = `Humidity - Last ${minutes} min (${chartData.dht.count} points)`;
          humidityChart.update('none');
        } else {
          console.log("No DHT data available for chart");
          // Clear charts if no data
          temperatureData.labels = [];
          temperatureData.datasets[0].data = [];
          humidityData.labels = [];
          humidityData.datasets[0].data = [];
          tempChart.options.plugins.title.text = `Temperature - Last ${minutes} min (0 points)`;
          humidityChart.options.plugins.title.text = `Humidity - Last ${minutes} min (0 points)`;
          tempChart.update('none');
          humidityChart.update('none');
        }
        
        // Update light chart
        if (chartData.ldr && chartData.ldr.count > 0) {
          console.log(`Updating light chart with ${chartData.ldr.count} LDR data points`);
          lightData.labels = [...chartData.ldr.labels];
          lightData.datasets[0].data = [...chartData.ldr.lightLevel];
          lightChart.options.plugins.title.text = `Light Level - Last ${minutes} min (${chartData.ldr.count} points)`;
          lightChart.update('none');
        } else {
          console.log("No LDR data available for chart");
          // Clear chart if no data
          lightData.labels = [];
          lightData.datasets[0].data = [];
          lightChart.options.plugins.title.text = `Light Level - Last ${minutes} min (0 points)`;
          lightChart.update('none');
        }
        
        console.log("Charts updated successfully");
        
      } else {
        console.warn("No chart data received or request failed:", response);
        // Try fallback method
        await loadHistoricalChartData();
      }
    } catch (error) {
      console.error("Error loading chart data:", error);
      // Try fallback method
      await loadHistoricalChartData();
    }
  }

  // Load historical chart data (fallback method)
  async function loadHistoricalChartData() {
    try {
      console.log("Loading historical chart data (fallback)...");
      
      // Load DHT data
      const dhtResponse = await fetchAuthenticatedAPI("/api/sensors/dht?limit=20");
      if (dhtResponse && dhtResponse.success && dhtResponse.data.length > 0) {
        const dhtData = dhtResponse.data.reverse(); // Reverse to show oldest first
        
        temperatureData.labels = dhtData.map(item => new Date(item.received_at).toLocaleTimeString());
        temperatureData.datasets[0].data = dhtData.map(item => item.temperature);
        humidityData.labels = dhtData.map(item => new Date(item.received_at).toLocaleTimeString());
        humidityData.datasets[0].data = dhtData.map(item => item.humidity);
        
        tempChart.options.plugins.title.text = `Temperature - Recent Data (${dhtData.length} points)`;
        humidityChart.options.plugins.title.text = `Humidity - Recent Data (${dhtData.length} points)`;
        tempChart.update();
        humidityChart.update();
        
        console.log("DHT historical data loaded");
      }

      // Load LDR data
      const ldrResponse = await fetchAuthenticatedAPI("/api/sensors/ldr?limit=20");
      if (ldrResponse && ldrResponse.success && ldrResponse.data.length > 0) {
        const ldrData = ldrResponse.data.reverse(); // Reverse to show oldest first
        
        lightData.labels = ldrData.map(item => new Date(item.received_at).toLocaleTimeString());
        lightData.datasets[0].data = ldrData.map(item => item.light_level);
        lightChart.options.plugins.title.text = `Light Level - Recent Data (${ldrData.length} points)`;
        lightChart.update();
        
        console.log("LDR historical data loaded");
      }
    } catch (error) {
      console.error("Error loading historical chart data:", error);
    }
  }

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
    const statusEl = document.getElementById("mqtt-connection-status");
    if (statusEl) {
      statusEl.textContent = "MQTT: Connected";
      statusEl.classList.add("connected");
    }

    // Subscribe to sensor data topic
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
    const statusEl = document.getElementById("mqtt-connection-status");
    if (statusEl) {
      statusEl.textContent = "MQTT: Error";
      statusEl.classList.remove("connected");
      statusEl.classList.add("error");
    }
  });

  mqttClient.on("disconnect", () => {
    console.log("Disconnected from MQTT broker");
    const statusEl = document.getElementById("mqtt-connection-status");
    if (statusEl) {
      statusEl.textContent = "MQTT: Disconnected";
      statusEl.classList.remove("connected");
    }
  });

  // Handle incoming MQTT messages
  mqttClient.on("message", (topic, message) => {
    if (topic === mqttTopic) {
      try {
        const payload = JSON.parse(message.toString());
        console.log("Received MQTT message:", payload);

        // Update timestamp and add as label
        const timestamp = new Date();
        const timeLabel = timestamp.toLocaleTimeString();

        // Check if it's DHT data
        if (payload.temperature !== undefined && payload.humidity !== undefined) {
          // Update UI elements
          const tempEl = document.getElementById("dht22-temp");
          const humidityEl = document.getElementById("dht22-humidity");
          const updateEl = document.getElementById("last-update");
          
          if (tempEl) tempEl.textContent = `${payload.temperature.toFixed(1)} °C`;
          if (humidityEl) humidityEl.textContent = `${payload.humidity.toFixed(1)} %`;
          if (updateEl) updateEl.textContent = timeLabel;

          // Add data to charts (real-time updates)
          if (temperatureData.labels.length >= maxDataPoints) {
            temperatureData.labels.shift();
            temperatureData.datasets[0].data.shift();
            humidityData.labels.shift();
            humidityData.datasets[0].data.shift();
          }

          temperatureData.labels.push(timeLabel);
          temperatureData.datasets[0].data.push(payload.temperature);
          humidityData.labels.push(timeLabel);
          humidityData.datasets[0].data.push(payload.humidity);

          // Update charts
          tempChart.update('none');
          humidityChart.update('none');
        }

        // Check if it's LDR data
        if (payload.light_level !== undefined) {
          const ldrEl = document.getElementById("ldr-value");
          const updateEl = document.getElementById("last-update");
          
          if (ldrEl) ldrEl.textContent = payload.light_level;
          if (updateEl) updateEl.textContent = timeLabel;

          // Add data to light chart
          if (lightData.labels.length >= maxDataPoints) {
            lightData.labels.shift();
            lightData.datasets[0].data.shift();
          }

          lightData.labels.push(timeLabel);
          lightData.datasets[0].data.push(payload.light_level);
          lightChart.update('none');
        }

      } catch (e) {
        console.error("Error parsing MQTT message:", e);
      }
    }
  });

  // LED Control
  const ledButton = document.getElementById("toggleLedButton");
  const ledStatusEl = document.getElementById("led-status");
  let isLedOn = false;

  if (ledButton && ledStatusEl) {
    ledButton.addEventListener("click", async () => {
      try {
        // Toggle LED state
        const newState = isLedOn ? 0 : 1;
        
        // Send API request to control LED
        const response = await fetchAuthenticatedAPI("/api/led/control", {
          method: "POST",
          body: JSON.stringify({ state: newState })
        });

        if (response && response.success) {
          isLedOn = !isLedOn;
          ledStatusEl.textContent = isLedOn ? "ON" : "OFF";
          ledStatusEl.style.color = isLedOn
            ? "var(--success-color, #2ecc71)"
            : "var(--error-color, #e74c3c)";
          
          console.log("LED control successful:", response);
        } else {
          console.error("LED control failed:", response);
        }
      } catch (error) {
        console.error("Error controlling LED:", error);
      }
    });
  }

  // Add time range selector for charts
  function addTimeRangeSelector() {
    // Try multiple possible locations for the selector
    let targetElement = document.querySelector('.dashboard-header') || 
                       document.querySelector('.dashboard-container') || 
                       document.querySelector('main') || 
                       document.body;
    
    if (targetElement) {
      const timeSelector = document.createElement('div');
      timeSelector.id = 'chart-time-selector';
      timeSelector.innerHTML = `
        <div style="margin: 10px 0; padding: 10px; background: #f5f5f5; border-radius: 5px;">
          <label for="timeRange" style="font-weight: bold; margin-right: 10px;">Chart Time Range: </label>
          <select id="timeRange" style="padding: 5px; border-radius: 4px; margin-right: 10px;">
            <option value="1">Last 1 minute</option>
            <option value="5">Last 5 minutes</option>
            <option value="10">Last 10 minutes</option>
            <option value="30">Last 30 minutes</option>
            <option value="60">Last 1 hour</option>
          </select>
          <button id="refreshCharts" style="padding: 5px 10px; background: #007bff; color: white; border: none; border-radius: 4px; cursor: pointer;">
            Refresh Charts
          </button>
          <span id="chart-status" style="margin-left: 10px; font-style: italic;"></span>
        </div>
      `;
      
      // Insert at the beginning of the target element
      targetElement.insertBefore(timeSelector, targetElement.firstChild);

      // Add event listeners with error handling
      const timeRangeSelect = document.getElementById('timeRange');
      const refreshButton = document.getElementById('refreshCharts');
      const statusSpan = document.getElementById('chart-status');
      
      if (timeRangeSelect) {
        timeRangeSelect.addEventListener('change', (e) => {
          const minutes = parseInt(e.target.value);
          console.log(`Time range changed to ${minutes} minutes`);
          if (statusSpan) statusSpan.textContent = 'Loading...';
          loadChartData(minutes).then(() => {
            if (statusSpan) statusSpan.textContent = `Updated: ${new Date().toLocaleTimeString()}`;
          });
        });
      }

      if (refreshButton) {
        refreshButton.addEventListener('click', () => {
          const minutes = parseInt(timeRangeSelect?.value || '1');
          console.log(`Manual refresh for ${minutes} minutes`);
          if (statusSpan) statusSpan.textContent = 'Refreshing...';
          loadChartData(minutes).then(() => {
            if (statusSpan) statusSpan.textContent = `Refreshed: ${new Date().toLocaleTimeString()}`;
          });
        });
      }
      
      console.log("Time range selector added successfully");
    } else {
      console.error("Could not find suitable element to add time range selector");
    }
  }

  // Initialize dashboard with proper sequencing
  async function initializeDashboard() {
    console.log("Initializing dashboard...");
    
    // Add time range selector first
    addTimeRangeSelector();
    
    // Load initial data
    await loadDashboardData();
    
    // Load initial chart data
    await loadChartData(1);
    
    console.log("Dashboard initialization complete");
  }

  // Start initialization
  initializeDashboard();

  // Refresh dashboard data every 30 seconds
  setInterval(() => {
    console.log("Auto-refreshing dashboard data...");
    loadDashboardData();
  }, 30000);
  
  // Refresh chart data every 10 seconds
  setInterval(() => {
    const timeRangeSelect = document.getElementById('timeRange');
    const minutes = parseInt(timeRangeSelect?.value || '1');
    console.log(`Auto-refreshing chart data for ${minutes} minutes...`);
    loadChartData(minutes);
  }, 10000);
});
