import { type Request, type Response, type NextFunction } from "express";
import jwt from "jsonwebtoken";
import jwtConfig from "@/configs/jwt.config";

// Define a custom Request type that includes the user property for type safety
export interface AuthenticatedRequest extends Request {
  user?: {
    userId: string;
    // You can add any other properties you include in your JWT payload here
    // For example: username?: string; role?: string;
  };
}

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
        [key: string]: any;
      };
      // Attach user information (e.g., userId) from token to the request object
      req.user = { userId: decoded.userId };
      next(); // Proceed to the next middleware or route handler
    } catch (error) {
      console.error("Token verification error:", error);
      res
        .status(401)
        .json({ message: "Not authorized, token failed or expired." });
      return; // Important to return after sending response
    }
  } else {
    res.status(401).json({ message: "Not authorized, no token provided." });
    return; // Important to return after sending response
  }
};
