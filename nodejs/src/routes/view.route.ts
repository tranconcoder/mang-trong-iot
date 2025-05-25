import { Router } from "express";
import { protectPage, redirectIfAuthenticated, type AuthenticatedRequest } from "@/middlewares/auth.middleware";

const router = Router();

/**
 * Route for the main Home page (after login)
 * @route GET /
 * @access Protected
 * @description Render the home page - requires authentication
 */
router.get("/", protectPage, (req: AuthenticatedRequest, res) => {
  res.render("home", {
    title: "Home",
    layout: "main",
    headerTitle: "Welcome Home",
    user: req.user, // Pass user data to template
  });
});

/**
 * Route for the Dashboard page
 * @route GET /dashboard
 * @access Protected
 * @description Render the dashboard page - requires authentication
 */
router.get("/dashboard", protectPage, (req: AuthenticatedRequest, res) => {
  res.render("dashboard", {
    title: "Dashboard",
    layout: "main",
    headerTitle: "IoT Sensor Dashboard",
    user: req.user, // Pass user data to template
  });
});

/**
 * Route for the login page
 * @route GET /login
 * @access Public
 * @description Render the login page - redirects if already authenticated
 */
router.get("/login", redirectIfAuthenticated, (req, res) => {
  const error = req.query.error as string;
  const returnUrl = req.query.returnUrl as string;
  
  let errorMessage = '';
  if (error === 'session_expired') {
    errorMessage = 'Your session has expired. Please log in again.';
  } else if (error === 'unauthorized') {
    errorMessage = 'Please log in to access this page.';
  }

  res.render("login", {
    title: "Admin Login",
    layout: "auth",
    error: errorMessage,
    returnUrl: returnUrl || '/dashboard',
  });
});

/**
 * Route for logout
 * @route GET /logout
 * @access Public
 * @description Clear authentication and redirect to login
 */
router.get("/logout", (req, res) => {
  // Clear the authentication cookie
  res.clearCookie('authToken');
  
  // Redirect to login with a success message
  res.redirect('/login?message=logged_out');
});

export default router;
