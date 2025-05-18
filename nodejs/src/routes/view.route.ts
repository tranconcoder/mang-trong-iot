import { Router } from "express";

const router = Router();

router.get("/", (req, res) => {
  res.render("login", { title: "Admin Login", layout: "auth" });
});

export default router;
