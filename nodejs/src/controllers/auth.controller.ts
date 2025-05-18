import authService from "@/services/auth.service";
import { type Request, type Response, type NextFunction } from "express";

export default new (class AuthController {
  public login = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    const { email, password } = req.body;

    if (!email || !password) {
      res.status(400).json({ message: "Email and password are required." });
      return;
    }

    try {
      const token = await authService.login(email, password);
      res.json({ token });
    } catch (error: any) {
      console.error("[AuthController] Login error:", error.message);

      if (error.message === "User not found") {
        res.status(404).json({
          message: "User not found. Please check your email or sign up.",
        });
      } else if (error.message === "Invalid password") {
        res
          .status(401)
          .json({ message: "Invalid password. Please try again." });
      } else {
        res
          .status(500)
          .json({ message: "An unexpected error occurred during login." });
      }
    }
  };
})();
