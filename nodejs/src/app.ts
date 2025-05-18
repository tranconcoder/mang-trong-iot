import express, { type Express, type Request, type Response } from "express";
import path from "path";
import morgan from "morgan";
import dotenv from "dotenv";
import { engine } from "express-handlebars";

import rootRoute from "@/routes";

const app = express();

dotenv.config({
  path: path.join(__dirname, "../.env.development"),
});

// Express middlewares
app.use(express.json());
app.use(express.urlencoded({ extended: true }));
app.use("/public", express.static(path.join(__dirname, "../public")));

// Custom middlewares
app.use(morgan("dev"));

// View engine
app.engine(
  "hbs",
  engine({
    extname: ".hbs",
    defaultLayout: "main",
    layoutsDir: path.join(__dirname, "views/layouts"),
    partialsDir: path.join(__dirname, "views/partials"),
  })
);
app.set("view engine", "hbs");
app.set("views", path.join(__dirname, "views"));

// Routes
app.use("/", rootRoute);

export default app;
