#!/usr/bin/env python3
"""
Real-Time Currency Exchange Rate Analyzer
A tool for scraping, analyzing, and visualizing currency exchange rate data
with real-time updates and data export capabilities.
"""

import requests
from bs4 import BeautifulSoup
import pandas as pd
import numpy as np
import re
import os
import time
import json
import argparse
from datetime import datetime
import matplotlib.pyplot as plt
import seaborn as sns
from urllib.parse import urljoin
import threading
import sqlite3
import warnings
warnings.filterwarnings('ignore')

# Optional imports for dashboard mode - we'll check for these later
dashboard_imports_available = True
try:
    import dash
    from dash import dcc, html, dash_table
    from dash.dependencies import Input, Output
    import dash_bootstrap_components as dbc
    import plotly.express as px
    import plotly.graph_objects as go
    from apscheduler.schedulers.background import BackgroundScheduler
except ImportError:
    dashboard_imports_available = False


class CurrencyAnalyzer:
    def __init__(self, base_url="https://tradingeconomics.com/currencies", db_path="currency_data.db"):
        self.base_url = base_url
        self.headers = {
            'User-Agent': 'Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/97.0.4692.71 Safari/537.36',
            'Accept-Language': 'en-US,en;q=0.9',
            'Accept-Encoding': 'gzip, deflate, br',
            'Connection': 'keep-alive',
            'Upgrade-Insecure-Requests': '1'
        }
        self.data = None
        self.df = None
        self.previous_df = None
        self.db_path = db_path
        self.setup_database()
        self.session_start_time = datetime.now().strftime("%Y%m%d_%H%M%S")
        self.csv_data_history = []
        self.json_data_history = []
        
    def setup_database(self):
        """Set up SQLite database for storing historical data."""
        conn = sqlite3.connect(self.db_path)
        cursor = conn.cursor()
        
        # Create tables if they don't exist
        cursor.execute('''
        CREATE TABLE IF NOT EXISTS currency_snapshots (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            timestamp TEXT NOT NULL,
            snapshot_data TEXT NOT NULL
        )
        ''')
        
        cursor.execute('''
        CREATE TABLE IF NOT EXISTS currency_history (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            currency_pair TEXT NOT NULL,
            price REAL,
            percent_change REAL,
            timestamp TEXT NOT NULL,
            UNIQUE(currency_pair, timestamp)
        )
        ''')
        
        conn.commit()
        conn.close()
        
    def save_to_database(self):
        """Save current data to database."""
        if self.df is None:
            print("No data to save to database")
            return
            
        conn = sqlite3.connect(self.db_path)
        
        # Save snapshot
        timestamp = datetime.now().isoformat()
        snapshot_json = self.df.to_json(orient='records')
        
        cursor = conn.cursor()
        cursor.execute(
            "INSERT INTO currency_snapshots (timestamp, snapshot_data) VALUES (?, ?)",
            (timestamp, snapshot_json)
        )
        
        # Save individual currency data points
        for _, row in self.df.iterrows():
            try:
                cursor.execute(
                    """INSERT OR REPLACE INTO currency_history 
                       (currency_pair, price, percent_change, timestamp) 
                       VALUES (?, ?, ?, ?)""",
                    (row['Currency Pair'], row['Price'], row['Percent Change'], timestamp)
                )
            except Exception as e:
                print(f"Error saving history for {row['Currency Pair']}: {e}")
        
        conn.commit()
        conn.close()
        print(f"Data saved to database at {timestamp}")
    
    def get_historical_data(self, currency_pair, limit=100):
        """Retrieve historical data for a specific currency pair."""
        conn = sqlite3.connect(self.db_path)
        query = f"""
        SELECT price, percent_change, timestamp
        FROM currency_history
        WHERE currency_pair = ?
        ORDER BY timestamp DESC
        LIMIT {limit}
        """
        
        df = pd.read_sql_query(query, conn, params=(currency_pair,))
        conn.close()
        
        # Convert timestamp to datetime
        df['timestamp'] = pd.to_datetime(df['timestamp'])
        return df
    
    def scrape_live_data(self):
        """Fetch live data from the website."""
        try:
            # Store previous data for change detection
            if self.df is not None:
                self.previous_df = self.df.copy()
                
            response = requests.get(self.base_url, headers=self.headers)
            response.raise_for_status()
            print(f"Successfully fetched live data from {self.base_url} at {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}")
            
            data = self._parse_html(response.text)
            
            # Save to database
            if data:
                self.save_to_database()
                
            # Detect significant changes
            if self.previous_df is not None and self.df is not None:
                self._detect_changes()
                
            return data
        except requests.exceptions.RequestException as e:
            print(f"Error fetching data: {e}")
            return None
    
    def _detect_changes(self, threshold=0.5):
        """Detect significant changes in currency rates."""
        if self.previous_df is None or self.df is None:
            return
        
        # Merge current and previous data
        merged = pd.merge(
            self.df[['Currency Pair', 'Price', 'Percent Change']], 
            self.previous_df[['Currency Pair', 'Price', 'Percent Change']], 
            on='Currency Pair', 
            suffixes=('_current', '_previous')
        )
        
        # Calculate absolute change in percentage
        merged['price_change_pct'] = abs(
            (merged['Price_current'] - merged['Price_previous']) / merged['Price_previous'] * 100
        )
        
        # Filter significant changes
        significant_changes = merged[merged['price_change_pct'] > threshold]
        
        if not significant_changes.empty:
            print("\n--- SIGNIFICANT CURRENCY MOVEMENTS DETECTED ---")
            for _, row in significant_changes.iterrows():
                direction = "up" if row['Price_current'] > row['Price_previous'] else "down"
                print(f"{row['Currency Pair']}: moved {direction} by {row['price_change_pct']:.2f}% " +
                      f"(from {row['Price_previous']:.4f} to {row['Price_current']:.4f})")
                      
        return significant_changes
    
    def _parse_html(self, html_content):
        """Parse HTML content to extract currency data."""
        soup = BeautifulSoup(html_content, 'html.parser')
        data = []
        
        # Find all currency tables
        tables = soup.find_all('table', {'class': lambda x: x and 'table-heatmap' in x.split()})
        
        for table in tables:
            # Get the region/group name
            group_input = table.find_previous('input', {'name': re.compile(r'.*group')})
            if group_input and 'value' in group_input.attrs:
                group = group_input['value']
            else:
                group = "Unknown Group"
            
            # Process data rows
            rows = table.find('tbody').find_all('tr')
            
            for row in rows:
                currency_data = {}
                
                # Add group
                currency_data['Group'] = group
                
                # Extract flag/country code
                flag_div = row.find('div', class_=lambda x: x and x.startswith('flag flag-'))
                country_code = None
                if flag_div and 'class' in flag_div.attrs:
                    for class_name in flag_div['class']:
                        if class_name.startswith('flag-'):
                            country_code = class_name.replace('flag-', '')
                currency_data['Country Code'] = country_code
                
                # Process all cells
                cells = row.find_all('td')
                
                # Extract currency pair
                currency_cell = cells[1] if len(cells) > 1 else None
                currency_pair = "Unknown"
                if currency_cell:
                    b_tag = currency_cell.find('b')
                    if b_tag:
                        currency_pair = b_tag.text.strip()
                currency_data['Currency Pair'] = currency_pair
                
                # Get price with proper formatting
                price_cell = cells[2] if len(cells) > 2 else None
                price = price_cell.text.strip() if price_cell else "N/A"
                currency_data['Price'] = self._clean_numeric(price)
                
                # Get day change
                day_change_cell = cells[3] if len(cells) > 3 else None
                day_change = day_change_cell.text.strip() if day_change_cell else "N/A"
                # Clean up by removing arrow symbols and extra spaces
                day_change = re.sub(r'[^\d.+-]', '', day_change)
                currency_data['Day Change'] = self._clean_numeric(day_change)
                
                # Get percent change
                percent_cell = cells[4] if len(cells) > 4 else None
                percent_change = percent_cell.text.strip() if percent_cell else "N/A"
                currency_data['Percent Change'] = self._clean_numeric(percent_change.replace('%', ''))
                
                # Get weekly, monthly, YTD, YoY changes
                if len(cells) > 5:
                    currency_data['Weekly'] = self._clean_numeric(cells[5].text.strip().replace('%', ''))
                if len(cells) > 6:
                    currency_data['Monthly'] = self._clean_numeric(cells[6].text.strip().replace('%', ''))
                if len(cells) > 7:
                    currency_data['YTD'] = self._clean_numeric(cells[7].text.strip().replace('%', ''))
                if len(cells) > 8:
                    currency_data['YoY'] = self._clean_numeric(cells[8].text.strip().replace('%', ''))
                
                # Add fetch timestamp
                currency_data['Fetch_Time'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
                
                # Get original timestamp if available
                if len(cells) > 9:
                    currency_data['Original_Timestamp'] = cells[9].text.strip()
                
                # Add all data to our list
                data.append(currency_data)
        
        self.data = data
        self._convert_to_dataframe()
        return data
    
    def _clean_numeric(self, value):
        """Clean numeric values and convert to float if possible."""
        if not value or value == "N/A":
            return None
        
        try:
            # Remove any non-numeric characters except decimal point and negative sign
            cleaned = re.sub(r'[^\d.-]', '', value)
            return float(cleaned)
        except (ValueError, TypeError):
            return value
    
    def _convert_to_dataframe(self):
        """Convert the parsed data to a pandas DataFrame."""
        if not self.data:
            print("No data to convert to DataFrame")
            return
        
        self.df = pd.DataFrame(self.data)
        
        # Add timestamp column
        if 'Fetch_Time' not in self.df.columns:
            self.df['Fetch_Time'] = datetime.now().strftime('%Y-%m-%d %H:%M:%S')
        
        # Convert numeric columns to float
        numeric_columns = ['Price', 'Day Change', 'Percent Change', 'Weekly', 'Monthly', 'YTD', 'YoY']
        for col in numeric_columns:
            if col in self.df.columns:
                self.df[col] = pd.to_numeric(self.df[col], errors='coerce')
        
        # Store in history for combined exports
        if not self.df.empty:
            self.csv_data_history.append(self.df.copy())
            self.json_data_history.append(self.df.copy())
    
    def save_to_csv(self, filename=None, append_mode=False):
        """Save the data to a CSV file.
        
        Args:
            filename (str, optional): CSV filename. If None, a default name will be used.
            append_mode (bool, optional): If True, append data to file instead of overwriting.
        """
        if self.df is None:
            print("No data to save")
            return False
            
        if not filename:
            # Use session timestamp for consistent file naming
            filename = f"currency_rates_session_{self.session_start_time}.csv"
            
        # Create a combined DataFrame from all fetches
        if append_mode and self.csv_data_history:
            combined_df = pd.concat(self.csv_data_history)
            # Write with or without header depending on if file exists
            header = not os.path.exists(filename)
            combined_df.to_csv(filename, index=False, mode='a', header=header)
        else:
            # Just save current data
            self.df.to_csv(filename, index=False)
            
        print(f"Data successfully saved to {filename}")
        return True
    
    def save_to_json(self, filename=None, append_mode=False):
        """Save the data to a JSON file.
        
        Args:
            filename (str, optional): JSON filename. If None, a default name will be used.
            append_mode (bool, optional): If True, save all historical data to the file.
        """
        if self.df is None:
            print("No data to save")
            return False
            
        if not filename:
            # Use session timestamp for consistent file naming
            filename = f"currency_rates_session_{self.session_start_time}.json"
            
        if append_mode and self.json_data_history:
            # Combine all historical data
            combined_df = pd.concat(self.json_data_history)
            # Group by fetch time for better organization
            grouped_data = combined_df.groupby('Fetch_Time')
            
            # Create a structured JSON with fetch timestamps as keys
            structured_data = {
                "session_id": self.session_start_time,
                "last_updated": datetime.now().isoformat(),
                "data": {}
            }
            
            # Add each fetch as a separate entry
            for fetch_time, group_data in grouped_data:
                structured_data["data"][fetch_time] = json.loads(group_data.to_json(orient='records'))
            
            # Write to file
            with open(filename, 'w', encoding='utf-8') as f:
                json.dump(structured_data, f, indent=2)
        else:
            # Just save current data
            self.df.to_json(filename, orient='records', indent=4)
            
        print(f"Data successfully saved to {filename}")
        return True
    
    def save_to_excel(self, filename=None):
        """Save the data to an Excel file."""
        if self.df is None:
            print("No data to save")
            return False
            
        if not filename:
            # Use session timestamp for consistent file naming
            filename = f"currency_rates_session_{self.session_start_time}.xlsx"
            
        # For Excel, we'll create a new file each time but include all historical data
        if self.csv_data_history:
            combined_df = pd.concat(self.csv_data_history)
            
            # Create Excel writer
            with pd.ExcelWriter(filename, engine='openpyxl') as writer:
                # Write combined data to "All Data" sheet
                combined_df.to_excel(writer, sheet_name='All Data', index=False)
                
                # Write current data to "Latest" sheet
                self.df.to_excel(writer, sheet_name='Latest', index=False)
                
                # Create sheets for each currency group
                if 'Group' in self.df.columns:
                    for group_name, group_data in self.df.groupby('Group'):
                        # Clean sheet name (remove invalid characters)
                        sheet_name = re.sub(r'[\\/*\[\]:?]', '', group_name)
                        if len(sheet_name) > 31:  # Excel sheet name length limit
                            sheet_name = sheet_name[:31]
                        group_data.to_excel(writer, sheet_name=sheet_name, index=False)
        else:
            # Just save current data
            self.df.to_excel(filename, index=False)
            
        print(f"Data successfully saved to {filename}")
        return True
    
    def get_top_performers(self, metric='Monthly', n=5):
        """Get the top performing currencies for a given metric."""
        if self.df is None:
            print("No data available")
            return None
        
        if metric not in ['Percent Change', 'Weekly', 'Monthly', 'YTD', 'YoY']:
            print(f"Invalid metric: {metric}. Using Monthly instead.")
            metric = 'Monthly'
        
        top_performers = self.df.dropna(subset=[metric]).nlargest(n, metric)
        return top_performers[['Currency Pair', 'Group', 'Price', metric]]
    
    def get_worst_performers(self, metric='Monthly', n=5):
        """Get the worst performing currencies for a given metric."""
        if self.df is None:
            print("No data available")
            return None
        
        if metric not in ['Percent Change', 'Weekly', 'Monthly', 'YTD', 'YoY']:
            print(f"Invalid metric: {metric}. Using Monthly instead.")
            metric = 'Monthly'
        
        worst_performers = self.df.dropna(subset=[metric]).nsmallest(n, metric)
        return worst_performers[['Currency Pair', 'Group', 'Price', metric]]


class RealTimeDashboard:
    def __init__(self, analyzer, refresh_interval=60):
        """Initialize the dashboard with the analyzer and refresh interval."""
        if not dashboard_imports_available:
            raise ImportError("Dashboard dependencies not installed. Run 'pip install dash dash-bootstrap-components plotly apscheduler'")
        
        self.analyzer = analyzer
        self.refresh_interval = refresh_interval  # in seconds
        self.app = dash.Dash(__name__, external_stylesheets=[dbc.themes.BOOTSTRAP])
        self.setup_layout()
        self.setup_callbacks()
        self.scheduler = BackgroundScheduler()
        
    def setup_layout(self):
        """Set up the dashboard layout."""
        self.app.layout = dbc.Container([
            dbc.Row([
                dbc.Col([
                    html.H1("Real-Time Currency Dashboard", className="display-4 text-center mb-4"),
                    html.Div(id="last-update-time", className="text-center mb-4"),
                ], width=12)
            ]),
            
            dbc.Row([
                dbc.Col([
                    dbc.Card([
                        dbc.CardHeader("Top & Bottom Performers"),
                        dbc.CardBody([
                            dbc.Tabs([
                                dbc.Tab([
                                    dcc.Graph(id="top-performers-chart")
                                ], label="Daily"),
                                dbc.Tab([
                                    dcc.Graph(id="top-performers-weekly-chart")
                                ], label="Weekly"),
                                dbc.Tab([
                                    dcc.Graph(id="top-performers-monthly-chart")
                                ], label="Monthly"),
                            ])
                        ])
                    ])
                ], width=6),
                
                dbc.Col([
                    dbc.Card([
                        dbc.CardHeader("Currency Group Performance"),
                        dbc.CardBody([
                            dcc.Graph(id="group-performance-chart")
                        ])
                    ])
                ], width=6)
            ], className="mb-4"),
            
            dbc.Row([
                dbc.Col([
                    dbc.Card([
                        dbc.CardHeader("Real-Time Currency Data"),
                        dbc.CardBody([
                            dbc.Row([
                                dbc.Col([
                                    dbc.InputGroup([
                                        dbc.InputGroupText("Search:"),
                                        dbc.Input(id="table-filter", placeholder="Filter by currency pair...", type="text"),
                                    ])
                                ], width=8),
                                dbc.Col([
                                    html.Div([
                                        dbc.Button("Download CSV", id="btn-download-csv", color="primary"),
                                        dcc.Download(id="download-csv")
                                    ], className="d-flex justify-content-end")
                                ], width=4)
                            ], className="mb-3"),
                            dash_table.DataTable(
                                id="currency-table",
                                page_size=15,
                                style_table={'overflowX': 'auto'},
                                style_data_conditional=[
                                    {
                                        'if': {'filter_query': '{Percent Change} > 0'},
                                        'backgroundColor': '#e6fff2',
                                        'color': 'green'
                                    },
                                    {
                                        'if': {'filter_query': '{Percent Change} < 0'},
                                        'backgroundColor': '#ffebe6',
                                        'color': 'red'
                                    }
                                ],
                                style_header={
                                    'backgroundColor': '#f8f9fa',
                                    'fontWeight': 'bold'
                                }
                            )
                        ])
                    ])
                ], width=12)
            ]),
            
            dbc.Row([
                dbc.Col([
                    dbc.Card([
                        dbc.CardHeader("Currency Pair Historical Chart"),
                        dbc.CardBody([
                            dbc.Row([
                                dbc.Col([
                                    dcc.Dropdown(
                                        id="currency-pair-dropdown",
                                        placeholder="Select a currency pair...",
                                    )
                                ], width=6)
                            ], className="mb-3"),
                            dcc.Graph(id="currency-history-chart")
                        ])
                    ])
                ], width=12)
            ], className="my-4"),
            
            dcc.Interval(
                id="interval-component",
                interval=self.refresh_interval * 1000,  # in milliseconds
                n_intervals=0
            )
        ], fluid=True, className="mt-4")
    
    def setup_callbacks(self):
        """Set up the dashboard callbacks."""
        # CSV download callback
        @self.app.callback(
            Output("download-csv", "data"),
            Input("btn-download-csv", "n_clicks"),
            prevent_initial_call=True,
        )
        def download_csv(n_clicks):
            if n_clicks is None:
                return None
                
            if self.analyzer.df is None:
                return None
                
            # Download the full history
            if self.analyzer.csv_data_history:
                combined_df = pd.concat(self.analyzer.csv_data_history)
                filename = f"currency_rates_session_{self.analyzer.session_start_time}.csv"
                return dcc.send_data_frame(combined_df.to_csv, filename=filename, index=False)
            else:
                # Just the current data if no history
                filename = f"currency_rates_latest.csv"
                return dcc.send_data_frame(self.analyzer.df.to_csv, filename=filename, index=False)
        
        # Update data on interval
        @self.app.callback(
            [Output("last-update-time", "children"),
             Output("currency-table", "data"),
             Output("currency-table", "columns"),
             Output("currency-pair-dropdown", "options")],
            [Input("interval-component", "n_intervals"),
             Input("table-filter", "value")]
        )
        def update_data(n_intervals, filter_value):
            # First fetch happens immediately on load, subsequent fetches on interval
            if n_intervals == 0 or self.analyzer.df is None:
                self.analyzer.scrape_live_data()
            
            if self.analyzer.df is None:
                return "No data available", [], [], []
            
            # Format last update time
            last_update = f"Last Updated: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}"
            
            # Filter data if filter_value is provided
            filtered_df = self.analyzer.df
            if filter_value:
                filtered_df = filtered_df[filtered_df['Currency Pair'].str.contains(filter_value, case=False)]
            
            # Format table data
            columns = [
                {"name": "Currency Pair", "id": "Currency Pair"}, 
                {"name": "Group", "id": "Group"},
                {"name": "Price", "id": "Price", "type": "numeric", "format": {"specifier": ".4f"}},
                {"name": "Daily Change %", "id": "Percent Change", "type": "numeric", "format": {"specifier": "+.2f"}},
                {"name": "Weekly %", "id": "Weekly", "type": "numeric", "format": {"specifier": "+.2f"}},
                {"name": "Monthly %", "id": "Monthly", "type": "numeric", "format": {"specifier": "+.2f"}}
            ]
            
            # Create dropdown options for currency pairs
            dropdown_options = [
                {"label": pair, "value": pair} for pair in sorted(self.analyzer.df['Currency Pair'].unique())
            ]
            
            return last_update, filtered_df.to_dict('records'), columns, dropdown_options
        
        # Update top performers charts
        @self.app.callback(
            [Output("top-performers-chart", "figure"),
             Output("top-performers-weekly-chart", "figure"),
             Output("top-performers-monthly-chart", "figure")],
            [Input("currency-table", "data")]
        )
        def update_top_performers(data):
            if not data or self.analyzer.df is None:
                empty_fig = go.Figure()
                empty_fig.update_layout(title="No data available")
                return empty_fig, empty_fig, empty_fig
            
            # Daily top performers
            daily_top = self.analyzer.get_top_performers(metric='Percent Change', n=5)
            daily_bottom = self.analyzer.get_worst_performers(metric='Percent Change', n=5)
            daily_fig = self._create_performers_chart(daily_top, daily_bottom, 'Percent Change', 'Daily')
            
            # Weekly top performers
            weekly_top = self.analyzer.get_top_performers(metric='Weekly', n=5)
            weekly_bottom = self.analyzer.get_worst_performers(metric='Weekly', n=5)
            weekly_fig = self._create_performers_chart(weekly_top, weekly_bottom, 'Weekly', 'Weekly')
            
            # Monthly top performers
            monthly_top = self.analyzer.get_top_performers(metric='Monthly', n=5)
            monthly_bottom = self.analyzer.get_worst_performers(metric='Monthly', n=5)
            monthly_fig = self._create_performers_chart(monthly_top, monthly_bottom, 'Monthly', 'Monthly')
            
            return daily_fig, weekly_fig, monthly_fig
        
        # Update group performance chart
        @self.app.callback(
            Output("group-performance-chart", "figure"),
            [Input("currency-table", "data")]
        )
        def update_group_performance(data):
            if not data or self.analyzer.df is None:
                empty_fig = go.Figure()
                empty_fig.update_layout(title="No data available")
                return empty_fig
            
            # Group performance analysis
            group_perf = self.analyzer.df.groupby('Group')[['Weekly', 'Monthly']].mean().reset_index()
            
            # Create a horizontal bar chart
            fig = go.Figure()
            
            # Add bars for Weekly performance
            fig.add_trace(go.Bar(
                y=group_perf['Group'],
                x=group_perf['Weekly'],
                name='Weekly',
                orientation='h',
                marker_color='rgb(26, 118, 255)'
            ))
            
            # Add bars for Monthly performance
            fig.add_trace(go.Bar(
                y=group_perf['Group'],
                x=group_perf['Monthly'],
                name='Monthly',
                orientation='h',
                marker_color='rgb(55, 83, 109)'
            ))
            
            fig.update_layout(
                title='Average Currency Performance by Group',
                xaxis_title='Performance (%)',
                yaxis=dict(
                    title='Currency Group',
                    categoryorder='total ascending'
                ),
                barmode='group',
                legend=dict(
                    orientation='h',
                    yanchor='bottom',
                    y=1.02,
                    xanchor='right',
                    x=1
                )
            )
            
            return fig
        
        # Update currency history chart
        @self.app.callback(
            Output("currency-history-chart", "figure"),
            [Input("currency-pair-dropdown", "value"),
             Input("interval-component", "n_intervals")]
        )
        def update_currency_history(currency_pair, n_intervals):
            if not currency_pair:
                empty_fig = go.Figure()
                empty_fig.update_layout(
                    title="Select a currency pair to view historical data",
                    xaxis=dict(title="Time"),
                    yaxis=dict(title="Price")
                )
                return empty_fig
            
            # Get historical data for the selected currency pair
            history_df = self.analyzer.get_historical_data(currency_pair)
            
            if history_df.empty:
                empty_fig = go.Figure()
                empty_fig.update_layout(
                    title=f"No historical data for {currency_pair}",
                    xaxis=dict(title="Time"),
                    yaxis=dict(title="Price")
                )
                return empty_fig
            
            # Sort by timestamp
            history_df = history_df.sort_values('timestamp')
            
            # Create line chart
            fig = go.Figure()
            
            # Add price line
            fig.add_trace(go.Scatter(
                x=history_df['timestamp'],
                y=history_df['price'],
                mode='lines+markers',
                name='Price',
                line=dict(color='rgb(0, 123, 255)', width=2)
            ))
            
            fig.update_layout(
                title=f'Historical Price for {currency_pair}',
                xaxis=dict(
                    title="Time",
                    tickformat='%Y-%m-%d %H:%M',
                    tickangle=45
                ),
                yaxis=dict(title="Price"),
                hovermode="x unified"
            )
            
            return fig
    
    def _create_performers_chart(self, top_df, bottom_df, metric, title_prefix):
        """Create a chart showing top and bottom performers."""
        if top_df is None or bottom_df is None or top_df.empty or bottom_df.empty:
            empty_fig = go.Figure()
            empty_fig.update_layout(title=f"No data available for {title_prefix} performers")
            return empty_fig
        
        # Combine top and bottom performers
        top_df['Type'] = 'Top'
        bottom_df['Type'] = 'Bottom'
        combined_df = pd.concat([top_df, bottom_df])
        
        # Create color mapping
        colors = []
        for value in combined_df[metric]:
            if value > 0:
                colors.append('green')
            else:
                colors.append('red')
        
        # Create the figure
        fig = go.Figure()
        
        # Add bars for top performers
        fig.add_trace(go.Bar(
            x=combined_df['Currency Pair'],
            y=combined_df[metric],
            marker_color=colors,
            text=combined_df[metric].apply(lambda x: f"{x:+.2f}%"),
            textposition='auto'
        ))
        
        fig.update_layout(
            title=f"{title_prefix} Top & Bottom Performers",
            xaxis=dict(
                title="Currency Pair",
                tickangle=45,
                categoryorder='total descending'
            ),
            yaxis=dict(title=f"{title_prefix} Change (%)"),
            height=500
        )
        
        return fig
    
    def start_data_updates(self):
        """Schedule regular data updates."""
        self.scheduler.add_job(
            self.analyzer.scrape_live_data, 
            'interval', 
            seconds=self.refresh_interval,
            id='fetch_currency_data'
        )
        self.scheduler.start()
        print(f"Scheduled data updates every {self.refresh_interval} seconds")
        
    def stop_data_updates(self):
        """Stop scheduled data updates."""
        self.scheduler.remove_job('fetch_currency_data')
        self.scheduler.shutdown()
        print("Stopped scheduled data updates")
        
    def run(self, debug=False, port=8050):
        """Run the dashboard."""
        try:
            print(f"Starting Real-Time Currency Dashboard on port {port}")
            self.start_data_updates()
            self.app.run_server(debug=debug, port=port)
        except KeyboardInterrupt:
            print("\nShutting down dashboard...")
        finally:
            self.stop_data_updates()


def main():
    """Main function to run the currency analyzer."""
    parser = argparse.ArgumentParser(description='Real-Time Currency Exchange Rate Analyzer')
    parser.add_argument('--refresh', type=int, default=60, help='Data refresh interval in seconds')
    parser.add_argument('--port', type=int, default=8050, help='Dashboard port')
    parser.add_argument('--db', type=str, default='currency_data.db', help='Database file path')
    parser.add_argument('--no-dashboard', action='store_true', help='Run without dashboard (CLI mode)')
    parser.add_argument('--save-csv', action='store_true', help='Save data to a single CSV file')
    parser.add_argument('--save-json', action='store_true', help='Save data to a single JSON file')
    parser.add_argument('--save-excel', action='store_true', help='Save data to Excel file')
    parser.add_argument('--output-dir', type=str, default='currency_data', help='Directory to save output files')
    
    args = parser.parse_args()
    
    # Create output directory if it doesn't exist
    if args.save_csv or args.save_json or args.save_excel:
        os.makedirs(args.output_dir, exist_ok=True)
    
    # Create the analyzer
    analyzer = CurrencyAnalyzer(db_path=args.db)
    
    # Initial data fetch
    print("Performing initial data fetch...")
    analyzer.scrape_live_data()
    
    # File paths for session files
    session_id = analyzer.session_start_time
    csv_path = os.path.join(args.output_dir, f"currency_rates_session_{session_id}.csv")
    json_path = os.path.join(args.output_dir, f"currency_rates_session_{session_id}.json")
    excel_path = os.path.join(args.output_dir, f"currency_rates_session_{session_id}.xlsx")
    
    # Save initial data if requested
    if args.save_csv:
        analyzer.save_to_csv(csv_path)
    
    if args.save_json:
        analyzer.save_to_json(json_path)
        
    if args.save_excel:
        analyzer.save_to_excel(excel_path)
    
    if args.no_dashboard:
        # CLI mode - just fetch and print data periodically
        try:
            while True:
                print("\nTop 5 performers (Percent Change):")
                print(analyzer.get_top_performers(metric='Percent Change', n=5))
                print("\nWorst 5 performers (Percent Change):")
                print(analyzer.get_worst_performers(metric='Percent Change', n=5))
                
                print(f"\nWaiting {args.refresh} seconds for next update...")
                time.sleep(args.refresh)
                analyzer.scrape_live_data()
                
                # Update the single files with all data
                if args.save_csv:
                    analyzer.save_to_csv(csv_path, append_mode=True)
                
                if args.save_json:
                    analyzer.save_to_json(json_path, append_mode=True)
                    
                if args.save_excel:
                    analyzer.save_to_excel(excel_path)
                
        except KeyboardInterrupt:
            print("\nExiting...")
    else:
        # Dashboard mode - check if required packages are available
        if not dashboard_imports_available:
            print("Dashboard dependencies not installed. Install with:")
            print("pip install dash dash-bootstrap-components plotly apscheduler")
            return
            
        # Start the dashboard
        dashboard = RealTimeDashboard(analyzer, refresh_interval=args.refresh)
        dashboard.run(debug=False, port=args.port)


if __name__ == "__main__":
    main()