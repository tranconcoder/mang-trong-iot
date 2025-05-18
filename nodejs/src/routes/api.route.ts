import authController from "@/controllers/auth.controller";
import { Router } from "express";
import {
  protectApi,
  type AuthenticatedRequest,
} from "@/middlewares/auth.middleware";

const router = Router();

router.post("/login", authController.login);

router.get(
  "/sensor-data/dht22",
  protectApi,
  (req: AuthenticatedRequest, res) => {
    res.json({
      message: "Successfully accessed protected DHT22 data!",
      user: req.user,
      data: {
        temperature: 25 + Math.random() * 2,
        humidity: 60 + Math.random() * 5,
      },
    });
  }
);

router.post("/led/toggle", protectApi, (req: AuthenticatedRequest, res) => {
  const currentStatus = Math.random() > 0.5 ? "ON" : "OFF";
  console.log("LED toggle request for user:", req.user?.userId);
  res.json({
    message: "LED status toggled (mock).",
    newStatus: currentStatus,
  });
});

export default router;
