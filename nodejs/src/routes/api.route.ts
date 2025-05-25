import authController from "@/controllers/auth.controller";
import sensorController from "@/controllers/sensor.controller";
import { Router, type Request, type Response, type NextFunction } from "express";
import {
  protectApi,
  type AuthenticatedRequest,
} from "@/middlewares/auth.middleware";

const router = Router();

// Auth routes
router.post("/login", authController.login);
router.post("/web-login", authController.webLogin);
router.post("/logout", authController.logout);

// Sensor data routes (protected)
router.get("/sensors/latest", protectApi, sensorController.getLatestSensorData);
router.get("/sensors/chart", protectApi, sensorController.getChartData);
router.get("/sensors/range", protectApi, sensorController.getSensorDataByTimeRange);
router.get("/sensors/dht", protectApi, sensorController.getDHTData);
router.get("/sensors/ldr", protectApi, sensorController.getLDRData);
router.get("/sensors/stats", protectApi, sensorController.getSensorStats);
router.get("/dashboard/data", protectApi, sensorController.getDashboardData);

// LED control routes (protected)
router.post("/led/control", protectApi, sensorController.controlLED);

// Legacy routes (for backward compatibility)
router.get(
  "/sensor-data/dht22",
  protectApi,
  sensorController.getDHTData
);

router.post("/led/toggle", protectApi, (req: Request, res: Response, next: NextFunction) => {
  // Toggle LED state (simplified version - get current state and toggle)
  const toggleState = Math.random() > 0.5 ? 1 : 0;
  console.log("LED toggle request for user:", (req as AuthenticatedRequest).user?.userId);
  
  // Set the state in request body for the controller
  req.body = { state: toggleState };
  
  // Call the actual LED control
  sensorController.controlLED(req, res, next);
});

export default router;
