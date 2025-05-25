import authService from "@/services/auth.service";
import { type Request, type Response, type NextFunction } from "express";

export default new (class AuthController {
  // API login endpoint
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
      const result = await authService.login(email, password);
      
      // Set HTTP-only cookie for web authentication
      res.cookie('authToken', result.token, {
        httpOnly: true,
        secure: process.env.NODE_ENV === 'production', // HTTPS in production
        sameSite: 'strict',
        maxAge: 24 * 60 * 60 * 1000, // 24 hours
      });
      
      res.json({ 
        token: result.token,
        user: result.user,
        message: "Login successful"
      });
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

  // Web login endpoint (for form submissions)
  public webLogin = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    const { email, password, returnUrl } = req.body;

    if (!email || !password) {
      res.redirect('/login?error=missing_credentials');
      return;
    }

    try {
      const result = await authService.login(email, password);
      
      // Set HTTP-only cookie for web authentication
      res.cookie('authToken', result.token, {
        httpOnly: true,
        secure: process.env.NODE_ENV === 'production',
        sameSite: 'strict',
        maxAge: 24 * 60 * 60 * 1000, // 24 hours
      });
      
      // Redirect to return URL or dashboard
      const redirectTo = returnUrl && returnUrl !== '/login' ? returnUrl : '/dashboard';
      res.redirect(redirectTo);
    } catch (error: any) {
      console.error("[AuthController] Web login error:", error.message);
      
      let errorParam = 'login_failed';
      if (error.message === "User not found") {
        errorParam = 'user_not_found';
      } else if (error.message === "Invalid password") {
        errorParam = 'invalid_password';
      }
      
      const returnUrlParam = returnUrl ? `&returnUrl=${encodeURIComponent(returnUrl)}` : '';
      res.redirect(`/login?error=${errorParam}${returnUrlParam}`);
    }
  };

  // Logout endpoint
  public logout = async (
    req: Request,
    res: Response,
    next: NextFunction
  ): Promise<void> => {
    // Clear the authentication cookie
    res.clearCookie('authToken');
    
    // Check if it's an API request or web request
    const acceptHeader = req.headers.accept;
    if (acceptHeader && acceptHeader.includes('application/json')) {
      // API request
      res.json({ message: "Logged out successfully" });
    } else {
      // Web request
      res.redirect('/login?message=logged_out');
    }
  };
})();
