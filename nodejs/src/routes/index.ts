import { Router } from "express";

// Routes
import apiRoute from "@/routes/api.route";
import viewRoute from "@/routes/view.route";

const router = Router();

/**
 * Api routes
 * @route /api
 * @description API routes
 * @method GET, POST, PUT, DELETE
 * @access Public
 */
router.use("/api", apiRoute);

/**
 * View routes
 * @route /
 * @description View routes
 * @method GET
 * @access Public
 */
router.use("/", viewRoute);

export default router;
