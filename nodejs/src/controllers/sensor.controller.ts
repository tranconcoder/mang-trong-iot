import { type Request, type Response, type NextFunction } from "express";
import mqttService from "@/services/mqtt.service";
import SensorData from "@/models/sensor-data.model";

export default new (class SensorController {
  // Get latest sensor data
  public getLatestSensorData = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    try {
      const { type, limit } = req.query;
      const dataLimit = limit ? parseInt(limit as string) : 10;
      
      const data = await mqttService.getLatestSensorData(
        type as string, 
        dataLimit
      );
      
      res.json({
        success: true,
        data: data,
        count: data.length
      });
    } catch (error: any) {
      console.error("[SensorController] Get latest sensor data error:", error.message);
      res.status(500).json({
        success: false,
        message: "Failed to fetch sensor data",
        error: error.message
      });
    }
  };

  // Get 1-minute history for chart drawing
  public getChartData = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    try {
      const { minutes = 1 } = req.query;
      const timeRange = parseInt(minutes as string);
      
      // Calculate time range (default 1 minute ago)
      const endTime = new Date();
      const startTime = new Date(endTime.getTime() - timeRange * 60 * 1000);
      
      // Get DHT data for the last minute
      const dhtData = await SensorData.find({
        data_type: "DHT",
        received_at: { $gte: startTime, $lte: endTime }
      })
      .sort({ received_at: 1 }) // Ascending order for chart
      .select('temperature humidity received_at timestamp')
      .lean();

      // Get LDR data for the last minute
      const ldrData = await SensorData.find({
        data_type: "LDR",
        received_at: { $gte: startTime, $lte: endTime }
      })
      .sort({ received_at: 1 }) // Ascending order for chart
      .select('light_level voltage light_status received_at timestamp')
      .lean();

      // Format data for charts
      const chartData = {
        timeRange: {
          start: startTime,
          end: endTime,
          minutes: timeRange
        },
        dht: {
          labels: dhtData.map(item => new Date(item.received_at).toLocaleTimeString()),
          temperature: dhtData.map(item => item.temperature),
          humidity: dhtData.map(item => item.humidity),
          timestamps: dhtData.map(item => item.received_at),
          count: dhtData.length
        },
        ldr: {
          labels: ldrData.map(item => new Date(item.received_at).toLocaleTimeString()),
          lightLevel: ldrData.map(item => item.light_level),
          voltage: ldrData.map(item => item.voltage),
          lightStatus: ldrData.map(item => item.light_status),
          timestamps: ldrData.map(item => item.received_at),
          count: ldrData.length
        }
      };

      res.json({
        success: true,
        data: chartData
      });
    } catch (error: any) {
      console.error("[SensorController] Get chart data error:", error.message);
      res.status(500).json({
        success: false,
        message: "Failed to fetch chart data",
        error: error.message
      });
    }
  };

  // Get sensor data by time range
  public getSensorDataByTimeRange = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    try {
      const { startTime, endTime, type } = req.query;
      
      if (!startTime || !endTime) {
        res.status(400).json({
          success: false,
          message: "startTime and endTime are required"
        });
        return;
      }

      const start = new Date(startTime as string);
      const end = new Date(endTime as string);
      
      const data = await mqttService.getSensorDataByTimeRange(
        start, 
        end, 
        type as string
      );
      
      res.json({
        success: true,
        data: data,
        count: data.length,
        timeRange: { startTime: start, endTime: end }
      });
    } catch (error: any) {
      console.error("[SensorController] Get sensor data by time range error:", error.message);
      res.status(500).json({
        success: false,
        message: "Failed to fetch sensor data by time range",
        error: error.message
      });
    }
  };

  // Get DHT data specifically
  public getDHTData = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    try {
      const { limit } = req.query;
      const dataLimit = limit ? parseInt(limit as string) : 20;
      
      const data = await SensorData.find({ data_type: "DHT" })
        .sort({ received_at: -1 })
        .limit(dataLimit)
        .select('temperature humidity timestamp received_at')
        .lean();
      
      res.json({
        success: true,
        data: data,
        count: data.length
      });
    } catch (error: any) {
      console.error("[SensorController] Get DHT data error:", error.message);
      res.status(500).json({
        success: false,
        message: "Failed to fetch DHT data",
        error: error.message
      });
    }
  };

  // Get LDR data specifically
  public getLDRData = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    try {
      const { limit } = req.query;
      const dataLimit = limit ? parseInt(limit as string) : 20;
      
      const data = await SensorData.find({ data_type: "LDR" })
        .sort({ received_at: -1 })
        .limit(dataLimit)
        .select('light_level voltage light_status timestamp received_at')
        .lean();
      
      res.json({
        success: true,
        data: data,
        count: data.length
      });
    } catch (error: any) {
      console.error("[SensorController] Get LDR data error:", error.message);
      res.status(500).json({
        success: false,
        message: "Failed to fetch LDR data",
        error: error.message
      });
    }
  };

  // Control LED
  public controlLED = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    try {
      const { state } = req.body;
      
      if (state === undefined || (state !== 0 && state !== 1)) {
        res.status(400).json({
          success: false,
          message: "LED state must be 0 (OFF) or 1 (ON)"
        });
        return;
      }

      // Publish LED control command via MQTT
      mqttService.publishLEDControl(state);
      
      res.json({
        success: true,
        message: `LED turned ${state ? 'ON' : 'OFF'}`,
        state: state
      });
    } catch (error: any) {
      console.error("[SensorController] LED control error:", error.message);
      res.status(500).json({
        success: false,
        message: "Failed to control LED",
        error: error.message
      });
    }
  };

  // Get dashboard data (latest from all sensors)
  public getDashboardData = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    try {
      // Get latest DHT data
      const latestDHT = await SensorData.findOne({ data_type: "DHT" })
        .sort({ received_at: -1 })
        .select('temperature humidity timestamp received_at')
        .lean();

      // Get latest LDR data
      const latestLDR = await SensorData.findOne({ data_type: "LDR" })
        .sort({ received_at: -1 })
        .select('light_level voltage light_status timestamp received_at')
        .lean();

      // Get latest LED state
      const latestLED = await SensorData.findOne({ data_type: "LED" })
        .sort({ received_at: -1 })
        .select('led_state timestamp received_at')
        .lean();

      // Format response similar to what the frontend expects
      const dashboardData = {
        sensors: {
          temperature: latestDHT?.temperature || 0,
          humidity: latestDHT?.humidity || 0,
          light: latestLDR?.light_level || 0
        },
        led: {
          state: latestLED?.led_state || 0,
          status: latestLED?.led_state ? "ON" : "OFF"
        },
        status: "online",
        lastUpdate: latestDHT?.received_at || latestLDR?.received_at || new Date(),
        timestamps: {
          dht: latestDHT?.received_at,
          ldr: latestLDR?.received_at,
          led: latestLED?.received_at
        }
      };

      res.json({
        success: true,
        data: dashboardData
      });
    } catch (error: any) {
      console.error("[SensorController] Get dashboard data error:", error.message);
      res.status(500).json({
        success: false,
        message: "Failed to fetch dashboard data",
        error: error.message
      });
    }
  };

  // Get sensor statistics
  public getSensorStats = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    try {
      const { hours } = req.query;
      const timeRange = hours ? parseInt(hours as string) : 24; // Default 24 hours
      
      const startTime = new Date(Date.now() - timeRange * 60 * 60 * 1000);
      
      // Get DHT statistics
      const dhtStats = await SensorData.aggregate([
        {
          $match: {
            data_type: "DHT",
            received_at: { $gte: startTime }
          }
        },
        {
          $group: {
            _id: null,
            avgTemperature: { $avg: "$temperature" },
            minTemperature: { $min: "$temperature" },
            maxTemperature: { $max: "$temperature" },
            avgHumidity: { $avg: "$humidity" },
            minHumidity: { $min: "$humidity" },
            maxHumidity: { $max: "$humidity" },
            count: { $sum: 1 }
          }
        }
      ]);

      // Get LDR statistics
      const ldrStats = await SensorData.aggregate([
        {
          $match: {
            data_type: "LDR",
            received_at: { $gte: startTime }
          }
        },
        {
          $group: {
            _id: null,
            avgLightLevel: { $avg: "$light_level" },
            minLightLevel: { $min: "$light_level" },
            maxLightLevel: { $max: "$light_level" },
            count: { $sum: 1 }
          }
        }
      ]);

      res.json({
        success: true,
        timeRange: `${timeRange} hours`,
        statistics: {
          dht: dhtStats[0] || null,
          ldr: ldrStats[0] || null
        }
      });
    } catch (error: any) {
      console.error("[SensorController] Get sensor stats error:", error.message);
      res.status(500).json({
        success: false,
        message: "Failed to fetch sensor statistics",
        error: error.message
      });
    }
  };
})(); 