import { Router } from "express";

const router = Router();

/**
 * Route for the main Home page (after login)
 * @route GET /
 * @access Public
 * @description Render the home page
 */
router.get("/", (req, res) => {
  res.render("home", {
    title: "Home",
    layout: "main",
    headerTitle: "Welcome Home",
  });
});

/**
 * Route for the Dashboard page
 * @route GET /dashboard
 * @access Public
 * @description Render the dashboard page
 */
router.get("/dashboard", (req, res) => {
  res.render("dashboard", {
    title: "Dashboard",
    layout: "main",
    headerTitle: "My Dashboard",
  });
});

/**
 * Route for the login page
 * @route GET /login
 * @access Public
 * @description Render the login page
 */
router.get("/login", (req, res) => {
  res.render("login", {
    title: "Admin Login",
    layout: "auth",
  });
});

export default router;
