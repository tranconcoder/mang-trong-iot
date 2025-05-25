import { type Request, type Response, type NextFunction } from "express";
import jwt from "jsonwebtoken";
import jwtConfig from "@/configs/jwt.config";

// Define a custom Request type that includes the user property for type safety
export interface AuthenticatedRequest extends Request {
  user?: {
    userId: string;
    email?: string;
    name?: string;
    // You can add any other properties you include in your JWT payload here
  };
}

// Middleware to protect API routes
export const protectApi = (
  req: AuthenticatedRequest,
  res: Response,
  next: NextFunction
): void => {
  const authHeader = req.headers.authorization;

  if (authHeader && authHeader.startsWith("Bearer ")) {
    const token = authHeader.split(" ")[1];
    try {
      const decoded = jwt.verify(token, jwtConfig.SECRET) as {
        userId: string;
        email?: string;
        name?: string;
        [key: string]: any;
      };
      // Attach user information from token to the request object
      req.user = { 
        userId: decoded.userId,
        email: decoded.email,
        name: decoded.name
      };
      next(); // Proceed to the next middleware or route handler
    } catch (error) {
      console.error("Token verification error:", error);
      res
        .status(401)
        .json({ message: "Not authorized, token failed or expired." });
      return;
    }
  } else {
    res.status(401).json({ message: "Not authorized, no token provided." });
    return;
  }
};

// Middleware to protect web pages (redirects to login)
export const protectPage = (
  req: AuthenticatedRequest,
  res: Response,
  next: NextFunction
): void => {
  // Check for token in cookies first, then in Authorization header
  const tokenFromCookie = req.cookies?.authToken;
  const authHeader = req.headers.authorization;
  const tokenFromHeader = authHeader && authHeader.startsWith("Bearer ") 
    ? authHeader.split(" ")[1] 
    : null;
  
  const token = tokenFromCookie || tokenFromHeader;

  if (token) {
    try {
      const decoded = jwt.verify(token, jwtConfig.SECRET) as {
        userId: string;
        email?: string;
        name?: string;
        [key: string]: any;
      };
      
      // Attach user information to request
      req.user = { 
        userId: decoded.userId,
        email: decoded.email,
        name: decoded.name
      };
      
      next(); // User is authenticated, proceed
    } catch (error) {
      console.error("Token verification error:", error);
      // Clear invalid cookie and redirect to login
      res.clearCookie('authToken');
      res.redirect('/login?error=session_expired');
      return;
    }
  } else {
    // No token found, redirect to login with return URL
    const returnUrl = encodeURIComponent(req.originalUrl);
    res.redirect(`/login?returnUrl=${returnUrl}`);
    return;
  }
};

// Middleware to redirect authenticated users away from login page
export const redirectIfAuthenticated = (
  req: AuthenticatedRequest,
  res: Response,
  next: NextFunction
): void => {
  const tokenFromCookie = req.cookies?.authToken;
  const authHeader = req.headers.authorization;
  const tokenFromHeader = authHeader && authHeader.startsWith("Bearer ") 
    ? authHeader.split(" ")[1] 
    : null;
  
  const token = tokenFromCookie || tokenFromHeader;

  if (token) {
    try {
      jwt.verify(token, jwtConfig.SECRET);
      // User is already authenticated, redirect to dashboard or return URL
      const returnUrl = req.query.returnUrl as string;
      const redirectTo = returnUrl && returnUrl !== '/login' ? returnUrl : '/dashboard';
      res.redirect(redirectTo);
      return;
    } catch (error) {
      // Invalid token, clear cookie and continue to login page
      res.clearCookie('authToken');
      next();
    }
  } else {
    // No token, continue to login page
    next();
  }
};
