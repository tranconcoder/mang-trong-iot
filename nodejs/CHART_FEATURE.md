# Chart Data Feature - 1 Minute History

This document describes the new chart data feature that allows the dashboard to display sensor data history for the last 1 minute (or configurable time range).

## Overview

The chart feature provides real-time visualization of sensor data with the ability to view historical data over different time ranges. It's designed to work with the existing MQTT integration and MongoDB storage.

## API Endpoint

### GET `/api/sensors/chart`

Retrieves sensor data for chart visualization over a specified time range.

**Query Parameters:**
- `minutes` (optional): Number of minutes to look back (default: 1)

**Response Format:**
```json
{
  "success": true,
  "data": {
    "timeRange": {
      "start": "2024-01-01T10:00:00.000Z",
      "end": "2024-01-01T10:01:00.000Z",
      "minutes": 1
    },
    "dht": {
      "labels": ["10:00:15", "10:00:30", "10:00:45"],
      "temperature": [25.5, 25.7, 25.6],
      "humidity": [60.2, 60.5, 60.3],
      "timestamps": ["2024-01-01T10:00:15.000Z", ...],
      "count": 3
    },
    "ldr": {
      "labels": ["10:00:20", "10:00:40"],
      "lightLevel": [2048, 2156],
      "voltage": [1.65, 1.73],
      "lightStatus": [1, 1],
      "timestamps": ["2024-01-01T10:00:20.000Z", ...],
      "count": 2
    }
  }
}
```

## Dashboard Integration

### Chart Configuration

The dashboard now includes enhanced chart configuration with:

- **Time Range Selector**: Dropdown to select 1, 5, 10, 30 minutes, or 1 hour
- **Auto-refresh**: Charts update every 10 seconds
- **Real-time Updates**: MQTT messages still update charts in real-time
- **Data Point Limits**: Maximum 20 data points for real-time updates

### Chart Features

1. **Historical Data Loading**: 
   - Loads data from MongoDB for the selected time range
   - Shows actual timestamps from database

2. **Real-time Updates**: 
   - MQTT messages add new data points
   - Maintains sliding window of recent data

3. **Interactive Controls**:
   - Time range selector (1 min to 1 hour)
   - Manual refresh button
   - Dynamic chart titles showing data count

### Chart Types

- **Temperature Chart**: Line chart showing temperature over time
- **Humidity Chart**: Line chart showing humidity percentage
- **Light Level Chart**: Line chart showing LDR sensor readings

## Database Queries

The chart endpoint uses optimized MongoDB queries:

```javascript
// DHT data query
const dhtData = await SensorData.find({
  data_type: "DHT",
  received_at: { $gte: startTime, $lte: endTime }
})
.sort({ received_at: 1 }) // Ascending for chart display
.select('temperature humidity received_at timestamp')
.lean();
```

### Indexes

The following indexes are recommended for optimal performance:

```javascript
// Existing indexes in sensor-data.model.ts
sensorDataSchema.index({ device_id: 1, data_type: 1, timestamp: -1 });
sensorDataSchema.index({ received_at: -1 });
sensorDataSchema.index({ node_address: 1, data_type: 1 });
```

## Usage Examples

### Frontend JavaScript

```javascript
// Load 1-minute chart data
async function loadChartData(minutes = 1) {
  const response = await fetchAuthenticatedAPI(`/api/sensors/chart?minutes=${minutes}`);
  if (response && response.success) {
    const chartData = response.data;
    
    // Update temperature chart
    temperatureData.labels = chartData.dht.labels;
    temperatureData.datasets[0].data = chartData.dht.temperature;
    tempChart.update('none');
  }
}

// Auto-refresh every 10 seconds
setInterval(() => {
  const minutes = parseInt(document.getElementById('timeRange').value);
  loadChartData(minutes);
}, 10000);
```

### API Testing

```bash
# Test with curl (requires authentication token)
curl -H "Authorization: Bearer YOUR_TOKEN" \
     "http://localhost:3000/api/sensors/chart?minutes=5"
```

## Performance Considerations

1. **Time Range Limits**: 
   - Recommended maximum: 1 hour (60 minutes)
   - Larger ranges may impact performance

2. **Data Point Optimization**:
   - Database queries are optimized with proper indexes
   - Only necessary fields are selected

3. **Update Frequency**:
   - Chart refreshes every 10 seconds
   - Real-time MQTT updates are immediate

## Error Handling

The endpoint handles various error scenarios:

- **No Data**: Returns empty arrays with count: 0
- **Invalid Time Range**: Uses default 1 minute
- **Database Errors**: Returns 500 status with error message
- **Authentication**: Requires valid JWT token

## Integration with MQTT

The chart feature works seamlessly with the existing MQTT integration:

1. **Data Storage**: MQTT messages are stored in MongoDB
2. **Real-time Updates**: New MQTT data appears in charts immediately
3. **Historical Context**: Charts show both historical and real-time data

## Future Enhancements

Potential improvements for the chart feature:

1. **Data Aggregation**: Average data points for longer time ranges
2. **Export Functionality**: Download chart data as CSV/JSON
3. **Zoom Controls**: Interactive chart zooming and panning
4. **Comparison Mode**: Compare data across different time periods
5. **Alert Thresholds**: Visual indicators for sensor value limits

## Troubleshooting

### Common Issues

1. **No Chart Data**: 
   - Check if MQTT is receiving data
   - Verify MongoDB connection
   - Check time range selection

2. **Slow Loading**:
   - Reduce time range
   - Check database indexes
   - Monitor server performance

3. **Authentication Errors**:
   - Verify JWT token is valid
   - Check token expiration
   - Ensure proper headers

### Debug Information

Enable debug logging to troubleshoot:

```javascript
console.log("Chart data loaded:", chartData);
console.log("DHT count:", chartData.dht.count);
console.log("LDR count:", chartData.ldr.count);
``` 